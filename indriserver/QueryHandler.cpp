//
// Created by Benjamin Piwowarski on 04/11/2016.
//

#include <algorithm>

#include "QueryHandler.h"
#include <websocketpp/utf8_validator.hpp>

#include <time.h>
#include "indri/QueryEnvironment.hpp"
#include "indri/LocalQueryServer.hpp"
#include "indri/delete_range.hpp"
#include "indri/NetworkStream.hpp"
#include "indri/NetworkMessageStream.hpp"
#include "indri/NetworkServerProxy.hpp"

#include "indri/ListIteratorNode.hpp"
#include "indri/ExtentInsideNode.hpp"
#include "indri/DocListIteratorNode.hpp"
#include "indri/FieldIteratorNode.hpp"

#include "indri/Parameters.hpp"

#include "indri/TaggedDocumentIterator.hpp"

#include "indri/RMExpander.hpp"
#include "indri/PonteExpander.hpp"
// need a QueryExpanderFactory....
#include "indri/TFIDFExpander.hpp"

#include "indri/ScopedLock.hpp"
#include "indri/SnippetBuilder.hpp"
#include "utils.h"

#include <queue>
#include <indri/IndexEnvironment.hpp>


using nlohmann::json;

static bool copy_parameters_to_string_vector(std::vector<std::string> &vec, json const &p,
                                             const std::string &parameterName) {
    if (p.count(parameterName) == 0)
        return false;

    json slice = p[parameterName];

    for (size_t i = 0; i < slice.size(); i++) {
        vec.push_back(slice[i]);
    }

    return true;
}

class query_t;

typedef std::shared_ptr<query_t> query_ptr;

struct query_t {
    struct greater {
        bool operator()(query_ptr const &one, query_ptr const &two) {
            return one->index > two->index;
        }
    };

    query_t(int _index, std::string _number, const std::string &_text, const std::string &queryType,
            std::vector<std::string> workSet, std::vector<std::string> FBDocs) :
            index(_index),
            number(_number),
            text(_text), qType(queryType), workingSet(workSet), relFBDocs(FBDocs) {
    }

    query_t(int _index, std::string _number, const std::string &_text) :
            index(_index),
            number(_number),
            text(_text) {
    }

    std::string number;
    int index;
    std::string text;
    std::string qType;
    // working set to restrict retrieval
    std::vector<std::string> workingSet;
    // Rel fb docs
    std::vector<std::string> relFBDocs;
    /// The query response in JSON
    json response;
};

indri::api::Parameters asIndriParameters(json const &p) {
    indri::api::Parameters ip;
    throw std::runtime_error("not implemented");
//    return ip;
}

typedef std::priority_queue<query_ptr, std::vector<query_ptr>, query_t::greater>
        QueryPriorityQueue;

static indri::parse::MetadataPair::key_equal docnoEq("docno");

class QueryThread : public indri::thread::UtilityThread {
private:
    indri::thread::Lockable &_queueLock;
    indri::thread::ConditionVariable &_queueEvent;
    std::queue<query_ptr> &_queries;
    QueryPriorityQueue &_output;

    indri::api::QueryEnvironment _environment;
    json const &_parameters;
    int _requested;
    int _initialRequested;

    bool _printPassages;
    bool _printSnippets;
    bool _printQuery;

    std::string _repository;

    indri::query::QueryExpander *_expander;
    std::vector<indri::api::ScoredExtentResult> _results;
    indri::api::QueryAnnotation *_annotation;

    // Runs the query, expanding it if necessary.  Will print output as well if verbose is on.
    void _runQuery(std::stringstream &output, const std::string &query,
                   const std::string &queryType, const std::vector<std::string> &workingSet,
                   std::vector<std::string> relFBDocs) {
        try {
            std::vector<lemur::api::DOCID_T> docids;;
            if (workingSet.size() > 0)
                docids = _environment.documentIDsFromMetadata("docno", workingSet);

            if (relFBDocs.size() == 0) {
                if (_printSnippets) {
                    if (workingSet.size() > 0)
                        _annotation = _environment.runAnnotatedQuery(query, docids, _initialRequested, queryType);
                    else
                        _annotation = _environment.runAnnotatedQuery(query, _initialRequested);
                    _results = _annotation->getResults();
                } else {
                    if (workingSet.size() > 0)
                        _results = _environment.runQuery(query, docids, _initialRequested, queryType);
                    else
                        _results = _environment.runQuery(query, _initialRequested, queryType);
                }
            }

            if (_expander) {
                std::vector<indri::api::ScoredExtentResult> fbDocs;
                if (relFBDocs.size() > 0) {
                    docids = _environment.documentIDsFromMetadata("docno", relFBDocs);
                    for (size_t i = 0; i < docids.size(); i++) {
                        indri::api::ScoredExtentResult r(0.0, docids[i]);
                        fbDocs.push_back(r);
                    }
                }
                std::string expandedQuery;
                if (relFBDocs.size() != 0)
                    expandedQuery = _expander->expand(query, fbDocs);
                else
                    expandedQuery = _expander->expand(query, _results);
                if (_printQuery) output << "# expanded: " << expandedQuery << std::endl;
                if (workingSet.size() > 0) {
                    docids = _environment.documentIDsFromMetadata("docno", workingSet);
                    _results = _environment.runQuery(expandedQuery, docids, _requested, queryType);
                } else {
                    _results = _environment.runQuery(expandedQuery, _requested, queryType);
                }
            }
        }
        catch (lemur::api::Exception &e) {
            _results.clear();
            LEMUR_RETHROW(e, "QueryThread::_runQuery Exception");
        }
    }

    void _printResultRegion(json &output, int start, int end) {
        std::vector<std::string> documentNames;
        std::vector<indri::api::ParsedDocument *> documents;

        std::vector<indri::api::ScoredExtentResult> resultSubset;

        resultSubset.assign(_results.begin() + start, _results.begin() + end);


        // Fetch document data for printing
        if (_printPassages || _printSnippets) {
            // Need document text, so we'll fetch the whole document
            documents = _environment.documents(resultSubset);
            documentNames.clear();

            for (size_t i = 0; i < resultSubset.size(); i++) {
                indri::api::ParsedDocument *doc = documents[i];
                std::string documentName;

                indri::utility::greedy_vector<indri::parse::MetadataPair>::iterator iter = std::find_if(
                        documents[i]->metadata.begin(),
                        documents[i]->metadata.end(),
                        docnoEq);

                if (iter != documents[i]->metadata.end())
                    documentName = (char *) iter->value;

                // store the document name in a separate vector so later code can find it
                documentNames.push_back(documentName);
            }
        } else {
            // We only want document names, so the documentMetadata call may be faster
            documentNames = _environment.documentMetadata(resultSubset, "docno");
        }

        std::vector<std::string> pathNames;

        // Print results
        for (size_t i = 0; i < resultSubset.size(); i++) {
            int rank = start + i + 1;
            json result = {
                    {"score", resultSubset[i].score},
                    {"docno", documentNames[i]},
                    {"id",    resultSubset[i].document},
                    {"begin", resultSubset[i].begin},
                    {"end",   resultSubset[i].end}
            };

            if (_printPassages) {
                int byteBegin = documents[i]->positions[resultSubset[i].begin].begin;
                int byteEnd = documents[i]->positions[resultSubset[i].end - 1].end;
                result["passage"] = std::string(documents[i]->text + byteBegin, byteEnd - byteBegin);
            }

            if (_printSnippets) {
                indri::api::SnippetBuilder builder(false);
                std::string snippet = builder.build(resultSubset[i].document, documents[i], _annotation);
                // Remove snippet if there are problems with UTF-8 string
                if (websocketpp::utf8_validator::validate(snippet)) {
                    result["snippet"] = snippet;
                }
            }

            output.push_back(result);

            if (documents.size())
                delete documents[i];
        }
    }

    void _printResults(json &output) {
        for (size_t start = 0; start < _results.size(); start += 50) {
            size_t end = std::min<size_t>(start + 50, _results.size());
            _printResultRegion(output, start, end);
        }
        delete _annotation;
        _annotation = 0;
    }


public:
    QueryThread(std::string const &repository,
                std::queue<query_ptr> &queries,
                QueryPriorityQueue &output,
                indri::thread::Lockable &queueLock,
                indri::thread::ConditionVariable &queueEvent,
                json &params) :
            _queries(queries),
            _output(output),
            _queueLock(queueLock),
            _queueEvent(queueEvent),
            _parameters(params),
            _expander(0),
            _annotation(0),
            _repository(repository) {
    }

    ~QueryThread() {
    }

    UINT64 initialize() {
        try {
            _environment.setSingleBackgroundModel(getOrDefault(_parameters, "singleBackgroundModel", false));

            std::vector<std::string> stopwords;
            if (copy_parameters_to_string_vector(stopwords, _parameters, "stopper.word"))
                _environment.setStopwords(stopwords);

            std::vector<std::string> smoothingRules;
            if (copy_parameters_to_string_vector(smoothingRules, _parameters, "rule"))
                _environment.setScoringRules(smoothingRules);

            _environment.addIndex(_repository);

            if (_parameters.count("maxWildcardTerms") > 0)
                _environment.setMaxWildcardTerms(_parameters["maxWildcardTerms"]);

            _requested = getOrDefault(_parameters, "count", 100);
            if (_requested > 1000) _requested = 1000;
            _initialRequested = getOrDefault(_parameters, "fbDocs", _requested);

            _printPassages = false; //getOrDefault(_parameters, "printPassages", false);
            _printSnippets = false; //getOrDefault(_parameters, "printSnippets", false);

            if (_parameters.count("baseline") > 0) {
                // doing a baseline
                std::string baseline = _parameters["baseline"];
                _environment.setBaseline(baseline);
                // need a factory for this...
                if (getOrDefault(_parameters, "fbDocs", 0) != 0) {
                    // have to push the method in...
                    std::string rule = "method:" + baseline;
                    getOrDefault(_parameters, "rule", rule);
                    auto p = asIndriParameters(_parameters);
                    _expander = new indri::query::TFIDFExpander(&_environment, p);
                }
            } else {
                if (getOrDefault(_parameters, "fbDocs", 0) != 0) {
                    auto p = asIndriParameters(_parameters);
                    _expander = new indri::query::RMExpander(&_environment, p);
                }
            }

            if (_parameters.count("maxWildcardTerms")) {
                _environment.setMaxWildcardTerms((int) _parameters["maxWildcardTerms"]);
            }
        } catch (lemur::api::Exception &e) {
            while (_queries.size()) {
                query_ptr query = _queries.front();
                _queries.pop();
                const shared_ptr<query_t> &v = std::make_shared<query_t>(query->index, query->number,
                                                                       "query: " + query->number +
                                                                       " QueryThread::_initialize exception\n");
                v->response = json(std::string("Error while answering query: ") + e.what());
                _output.push(v);
                _queueEvent.notifyAll();
                LEMUR_RETHROW(e, "QueryThread::_initialize");
            }
        }  catch (std::exception &e) {
            while (_queries.size()) {
                query_ptr query = _queries.front();
                _queries.pop();
                const shared_ptr<query_t> &v = std::make_shared<query_t>(query->index, query->number,
                                                                       "query: " + query->number +
                                                                       " QueryThread::_initialize exception\n");
                v->response = json(std::string("Error while answering query: ") + e.what());
                _output.push(v);
                _queueEvent.notifyAll();
                LEMUR_THROW(LEMUR_BAD_PARAMETER_ERROR, "QueryThread::_initialize");

            }
        }
        return 0;
    }

    void deinitialize() {
        delete _expander;
        _environment.close();
    }

    bool hasWork() {
        indri::thread::ScopedLock sl(&_queueLock);
        return _queries.size() > 0;
    }

    UINT64 work() {
        query_ptr query;
        std::stringstream output;

        // pop a query off the queue
        {
            indri::thread::ScopedLock sl(&_queueLock);
            if (_queries.size()) {
                query = _queries.front();
                _queries.pop();
            } else {
                return 0;
            }
        }

        // run the query
        try {
            if (_parameters.count("baseline") > 0 &&
                ((query->text.find("#") != std::string::npos) || (query->text.find(".") != std::string::npos))) {
                LEMUR_THROW(LEMUR_PARSE_ERROR, "Can't run baseline on this query: " + query->text +
                                               "\nindri query language operators are not allowed.");
            }
            _runQuery(output, query->text, query->qType, query->workingSet, query->relFBDocs);
        } catch (lemur::api::Exception &e) {
            output << "# EXCEPTION in query " << query->number << ": " << e.what() << std::endl;
        }

        // Add the results
        query->response = json::array();
        _printResults(query->response);

        // push that data into an output queue...?
        {
            indri::thread::ScopedLock sl(&_queueLock);
            _output.push(query);
            _queueEvent.notifyAll();
        }

        return 0;
    }
};

void push_queue(std::queue<query_ptr> &q, json const &queries,
                int queryOffset) {

    for (size_t i = 0; i < queries.size(); i++) {
        std::string queryNumber;
        std::string queryText;
        std::string queryType = "indri";
        if (queries[i].count("type") > 0)
            queryType = queries[i]["type"];
        if (queries[i].count("text") > 0)
            queryText = queries[i]["text"];
        if (queries[i].count("number") > 0) {
            queryNumber = queries[i]["number"];
        } else {
            int thisQuery = queryOffset + int(i);
            queryNumber = std::to_string(thisQuery);
        }
        if (queryText.size() == 0)
            queryText = queries[i];

        // working set and RELFB docs go here.
        // working set to restrict retrieval
        std::vector<std::string> workingSet;
        // Rel fb docs
        std::vector<std::string> relFBDocs;
        copy_parameters_to_string_vector(workingSet, queries[i], "workingSetDocno");
        copy_parameters_to_string_vector(relFBDocs, queries[i], "feedbackDocno");

        q.push(std::make_shared<query_t>(i, queryNumber, queryText, queryType, workingSet, relFBDocs));

    }
}

void handleQuery(std::string const &repository, Request &request, json &parameters) {
    try {
        if (request.assertion(parameters.count("baseline") == 0 || parameters.count("rule") == 0,
                              "Smoothing rules may not be specified when running a baseline."))
            return;

        json parameterQueries = parameters["query"];
        std::size_t threadCount = 4;
        if (parameterQueries.size() < 4) threadCount = parameterQueries.size();

        std::queue<query_ptr> queries;
        QueryPriorityQueue output;
        std::vector<std::unique_ptr<QueryThread>> threads;
        indri::thread::Mutex queueLock;
        indri::thread::ConditionVariable queueEvent;

        // push all queries onto a queue
        int queryOffset = getOrDefault(parameters, "queryOffset", 0);
        push_queue(queries, parameterQueries, queryOffset);
        int queryCount = (int) queries.size();

        // launch threads
        for (int i = 0; i < threadCount; i++) {
            threads.push_back(std::unique_ptr<QueryThread>(
                    new QueryThread(repository, queries, output, queueLock, queueEvent, parameters)));
            threads.back()->start();
        }

        int query = 0;

        // acquire the lock.
        queueLock.lock();

        json response = json::object();

        // process output as it appears on the queue
        while (query < queryCount) {
            query_ptr result = NULL;

            // wait for something to happen
            queueEvent.wait(queueLock);

            while (output.size() && output.top()->index == query) {
                result = output.top();
                output.pop();

                queueLock.unlock();

                response[result->number] = result->response;
                query++;

                queueLock.lock();
            }
        }
        queueLock.unlock();

        // join all the threads
        for (size_t i = 0; i < threads.size(); i++)
            threads[i]->join();

        request.send(response.dump());
    } catch (lemur::api::Exception &e) {
        request.assertion(false, e.what());
    } catch (std::exception &e) {
        request.assertion(false, e.what());
    } catch (...) {
        request.assertion(false, "unhandled exception");
    }
}

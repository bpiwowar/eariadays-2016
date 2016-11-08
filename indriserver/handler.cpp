//
// Created by Benjamin Piwowarski on 04/11/2016.
//

#include <iostream>
#include <queue>
#include <regex>
#include <boost/locale.hpp>
#include <indri/LocalQueryServer.hpp>

#include "handler.h"
#include "indri/IndexEnvironment.hpp"
#include "indri/CompressedCollection.hpp"

#include "json.hpp"
#include "QueryHandler.h"
#include "utils.h"
#include "Leaderboard.h"

using nlohmann::json;

std::regex re_meta_charset("<meta.*?charset=([^\"']+)",
                           std::regex_constants::icase);

bool Request::assertion(bool value, std::string const &message) {
    if (!value) {
        json response = {{"error", true},
                         {"message", message}};
        send(response.dump());
    }
    return !value;
}

class _IndriServer {
    std::string _repositoryPath;
    Leaderboard _leaderboard;
    size_t _ndocuments;
public:
    _IndriServer(const std::string &path) : _repositoryPath(path), _leaderboard(path + "/leaderboard") {
        indri::collection::Repository _repository;
        _repository.openRead(_repositoryPath);
        _ndocuments = 0;

        for (auto index: *_repository.indexes()) {
            _ndocuments += index->documentCount();
        }
        std::cerr << "# of documents " << _ndocuments << std::endl;
    }

    void query(Request &request, json &message) {
        handleQuery(_repositoryPath, request, message);
    }

    /// Get document (raw or pre-processed)
    void document(Request &request, json const &message) {
        const bool binary = getOrDefault<bool>(message, "binary", false);
        std::string type = getOrDefault<std::string>(message, "type", "raw");
        lemur::api::DOCID_T documentID = message["docid"];
        std::cerr << "Retrieving document " << documentID << std::endl;
        indri::collection::Repository _repository;
        _repository.openRead(_repositoryPath);
        if (request.assertion(documentID > 0 && documentID < _ndocuments, "Unknown document")) return;

        if (type == "raw") {
            indri::collection::CompressedCollection *collection = _repository.collection();
            std::unique_ptr<indri::api::ParsedDocument> document(collection->retrieve(documentID));
            if (request.assertion((bool) document, "Unknown document")) return;

            // Convert - find encoding first...
            std::string documentString(document->text);
            auto words_begin = std::sregex_iterator(documentString.begin(), documentString.end(), re_meta_charset);
            auto words_end = std::sregex_iterator();
            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::string encoding = (*i)[1];
                documentString = boost::locale::conv::to_utf<char>(documentString, encoding);
                break;
            }


            if (binary)
                request.sendBinary(json(documentString).dump());
            else
                request.send(json(documentString).dump());
        } else if (type == "bow") {
            indri::server::LocalQueryServer local(_repository);

//            std::vector<lemur::api::DOCID_T> documentIDs = { documentID };
//            documentIDs.push_back(documentID);

            std::unique_ptr<indri::server::QueryServerVectorsResponse> response(
                    local.documentVectors({documentID}));

            json fields = json::array();
            json occurrences = json::array();

            if (response->getResults().size()) {
                std::unique_ptr<indri::api::DocumentVector> docVector(response->getResults()[0]);
                if (request.assertion((bool) docVector, "Unknown document")) return;

                // Fields
                for (size_t i = 0; i < docVector->fields().size(); i++) {
                    const indri::api::DocumentVector::Field &field = docVector->fields()[i];
                    fields.push_back(
                            {
                                    {"name",   field.name},
                                    {"begin",  field.begin},
                                    {"end",    field.end},
                                    {"number", field.number}
                            }
                    );
                }

                for (size_t i = 0; i < docVector->positions().size(); i++) {
                    int termId = docVector->positions()[i];
                    const std::string &stem = docVector->stems()[termId];
                    occurrences.push_back({termId, stem});
                }

            }
            json answer = {{"fields",      std::move(fields)},
                           {"occurrences", std::move(occurrences)}};

            if (binary)
                request.sendBinary(answer.dump());
            else
                request.send(answer.dump());
        } else {
            request.assertion(false, "Unknown document type requested: " + type);
        }
    }

    void docid(Request &request, json command) {
        json response = json::array();
        indri::collection::Repository _repository;
        _repository.openRead(_repositoryPath);

        std::unique_ptr<indri::collection::CompressedCollection> collection(_repository.collection());
        std::string attributeName = "docno";
        std::string attributeValue = command["docno"];
        std::vector<lemur::api::DOCID_T> documentIDs;

        documentIDs = collection->retrieveIDByMetadatum(attributeName, attributeValue);

        for (size_t i = 0; i < documentIDs.size(); i++) {
            response.push_back(documentIDs[i]);
        }
        request.send(response.dump());
    }

    void submit(Request &request, json command) {
        if (request.assertion(command.count("name") > 0, "name not given")) return;
        if (request.assertion(command.count("run") > 0, "name not given")) return;
        if (request.assertion(command.count("password") > 0, "name not given")) return;

        std::string run = command["run"];
        std::string name = command["name"];
        std::string password = command["password"];

        request.send(_leaderboard.submit(run, name, password).dump());
    }

    void leaderboard(Request &request, json command) {
        request.send(_leaderboard.status().dump());
    }
};

IndriServer::IndriServer(std::string const &path) {
    _indri = new _IndriServer(path);
}

IndriServer::~IndriServer() {
    delete _indri;
}


void IndriServer::handle(Request &request) {
//    std::cerr << request.message() << std::endl;
    try {
        json command;
        try {
            command = json::parse(request.message());
        } catch (std::invalid_argument &e) {
            request.assertion(false, e.what());
            return;
        }

        if (request.assertion(command.is_object(), "Expected a JSON object")) return;
        if (request.assertion(command.count("command") > 0, "Expected a command")) return;

        std::string id = command.at("command");

        if (id == "query") {
            _indri->query(request, command);
        } else if (id == "document") {
            _indri->document(request, command);
        } else if (id == "docid") {
            _indri->docid(request, command);
        } else if (id == "submit") {
            _indri->submit(request, command);
        } else if (id == "leaderboard") {
            _indri->leaderboard(request, command);
        } else {
            request.assertion(false, "Unknown command " + id);
        }
    } catch (std::exception &e) {
        request.assertion(false, e.what());
    } catch (lemur::api::Exception &e) {
        request.assertion(false, e.what());
    }

}
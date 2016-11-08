import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.File;
import java.io.IOException;

public class ReadSessions {



    public static void main(String[] args) {





        try{
            // creation d'une fabrique de documents
            DocumentBuilderFactory fabrique = DocumentBuilderFactory.newInstance();

            // creation d'un constructeur de documents
            DocumentBuilder constructeur = fabrique.newDocumentBuilder();

            // lecture du contenu d'un fichier XML avec DOM
            File xml = new File(args[0]);
            Document document = constructeur.parse(xml);

            //traitement du document
            Element racine = document.getDocumentElement();

            NodeList sessions = racine.getElementsByTagName("session");
            System.out.println("Il y a :"+sessions.getLength()+" sessions");
            for (int i=0;i<sessions.getLength();i++) {
                Node session = sessions.item(i);
                System.out.print("Session "+session.getAttributes().getNamedItem("num").getNodeValue()+" - " );
                // récupération du numero de topic
                int numtopic = Integer.parseInt(((Element) session).getElementsByTagName("topic").item(0).getAttributes().getNamedItem("num").getNodeValue());
                System.out.print("Numtopic "+numtopic+" - ");
                // récupération du nombre d'interactions
                int nbInteractions = ((Element) session).getElementsByTagName("interaction").getLength();
                System.out.println("Nb interactions "+nbInteractions);
                //affichage de la currentQuery si elle existe
                if (((Element) session).getElementsByTagName("currentquery").getLength()>0){
                    Node currentquery = ((Element) session).getElementsByTagName("currentquery").item(0);
                    System.out.println("\t Currentquery :" + ((Element) currentquery).getElementsByTagName("query").item(0).getTextContent());
                }

            }



        }catch(ParserConfigurationException pce){
            System.out.println("Erreur de configuration du parseur DOM");
            System.out.println("lors de l'appel à fabrique.newDocumentBuilder();");
        }catch(SAXException se){
            System.out.println("Erreur lors du parsing du document");
            System.out.println("lors de l'appel à construteur.parse(xml)");
        }catch(IOException ioe){
            System.out.println("Erreur d'entrée/sortie");
            System.out.println("lors de l'appel à construteur.parse(xml)");
        }
    }




}



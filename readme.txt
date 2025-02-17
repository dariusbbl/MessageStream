Pentru realizarea temei, m-am folosit de 3 fisiere sursa: server.cpp, subscriber.cpp si helpers.h

server.cpp:
    Codul acestui fisier reprezinta sursa pentru serverul de retea ce gestioneaza conexiuni TCP si UDP 
    pentru comunicarea cu clientii si transmiterea de mesaje. Pentru TCP, serverul accepta conexiuni 
    de la clienti, primeste mesaje si gestioneaza starea de conectare a fiecarui client. Pentru UDP, 
    serverul primeste pachete de date care sunt procesate si apoi trimise catre clientii abonati la 
    anumite topicuri, folosind conexiunile TCP stabilite.
    Pentru gestionarea clientilor am folosit structura de date "unbordered_map" pentru a-i identifica 
    prin ID-ul unic si sunt asociati cu socket TCp si cu starea sa (online/offline)
    Clientii pot fi abonati la diferite topicuri si pot primi actualizari prin intermediul conexiunii TCP. 
    Serverul proceseaza mesajele UDP primite, le transforma in format TCP si le distribuie clientilor 
    abonati corespunzatori.
    -> info_client: structura ce stocheaza informatii despre fiecare client, inclusiv starea sa si socketul asociat
    -> handle_client(): functie ce gestioneaza conexiunile noi si verifica daca un client este deja conectat
    -> send_to_subscribers(): trimite mesaje clientilor abonati, bazandu-se pe topicurile la care sunt abonati
    -> extract_udp_message(): transforma diferitele tipuri de date din mesajele UDP (INT, SHORT_REAL, FLOAT, STRING) 
    intr-un format potrivit pentru clientii TCP. De asemenea, identifica si gestioneaza posibilele erori sau formatari incorecte ale datelor venite prin UDP
    -> handle_stdin(): poate executa comenzi cum ar fi "shutdown", "restart" sau "status" si permite ajustarea setarilor serverului in timp real, 
    fara a necesita restartul aplicatiei
    -> run(): contine bucla principala de executie a serverului, coordonand toate activitatile de retea si de procesare a datelor

subscriber.cpp:
    Codul acestui fisier este destinat clientului TCP care se conecteaza la server, trimite comenzi de abonare 
    si dezabonare la diferite topicuri si primeste mesaje asociate acestora de la server. 
    -> send_request(): ofera clientului posibilitatea de a trimite comenzi de abonare/dezabonare la topicuri
    -> print_contents(): afiseaza mesajele primite de la server: sursa, topicul si continutul mesajului
    -> run():  clientul mentine un ciclu continuu de asteptare, utilizand functia select() pentru a alterna intre preluarea 
    inputului de la utilizator si receptia mesajelor de la server
    -> select(): select(): folosit pentru a astepta si a reacționa la date disponibile fie de la tastatura (STDIN_FILENO), 
    fie de pe socketul conectat la server

helpers.h:
    Acest file include definitii si structuri utile pentru gestionarea cerintei generale,precum:
    -> Macro-ul DIE luat dintr-un laborator trecut
    -> Structurile "udp_message" si "tcp_message" unt utilizate pentru a defini formatul mesajelor ce sunt transmise prin protocoalele UDP si TCP

 


1. computeAverageDegree

Cosa fa:
Calcola il grado medio (cioè la media del numero di archi incidenti) dei vertici del grafo.

Come funziona:
	•	Ottiene il numero totale dei vertici n del grafo.
	•	Itera su tutti i vertici, sommando il grado (numero di adiacenze) di ciascun vertice.
	•	Ritorna la media: se n > 0 la somma divisa per n, altrimenti 0.


Immagina un grafo con 4 vertici con gradi rispettivi: 2, 3, 3, 2.
La funzione calcolerà:
	•	Somma totale = 2 + 3 + 3 + 2 = 10
	•	Grado medio = 10 / 4 = 2.5

2. computeNeighborBitsets

Cosa fa:
Precalcola, per ogni vertice, un bitset che indica quali vertici sono suoi vicini.

Come funziona:
	•	i vertici sono etichettati da 0 a n-1.
	•	Per ogni vertice v, crea un bitset di dimensione MAX_VERTICES.
	•	Itera sui vertici adiacenti a v e imposta a true il bit corrispondente al vicino.



3. improvedParallelGreedyCliqueExpansionAll

Cosa fa:
Trova una clique del grafo usando una strategia greedy, sfruttando i bitset per l’intersezione dei vicini, ed eseguendola in parallelo con OpenMP per velocizzare la ricerca.

Come funziona:
	1.	Preparazione:
	•	Recupera tutti i vertici del grafo e li inserisce in vertexList.
	•	Per ogni vertice, calcola il grado (numero di adiacenze) e lo memorizza in un vettore degrees.
	•	Ordina vertexList in ordine discendente di grado. Questo aiuta a partire dai vertici più “connessi”, aumentando la probabilità di espandere clique più grandi.
	2.	Parallelizzazione:
	•	Avvia una regione parallela OpenMP. Ogni thread mantiene una propria variabile locale threadBestClique per memorizzare la migliore clique trovata da quel thread.
	3.	Espansione Greedy (per ogni vertice):
	•	Per ogni vertice v della lista ordinata, eseguito in parallelo:
	•	Inizializzazione della Clique:
Crea currentClique inizializzata con il solo vertice v.
	•	Inizializzazione dei Candidati:
Copia il bitset dei vicini di v in cliqueCandidates. Questo bitset rappresenta i possibili candidati da aggiungere alla clique.
	•	Ordinamento dei Vicini:
Estrae i vicini di v in un vettore e li ordina in ordine decrescente di grado.
	•	Iterazione sui Candidati:
Per ogni vicino u in quest’ordine:
	•	Se u è ancora presente in cliqueCandidates (cioè il bit corrispondente a u è true), allora:
	•	Aggiunge u a currentClique.
	•	Aggiorna cliqueCandidates con l’intersezione:
cliqueCandidates &= neighborBitsets[u]
Questo mantiene solo quei vertici che sono adiacenti a tutti i vertici già presenti nella clique.
	•	Aggiornamento della Migliore Clique Locale:
Se la clique corrente è più grande di quella migliore trovata dal thread, la sostituisce.
	4.	Riduzione Finale:
	•	In una sezione critica (protetta da OpenMP), ogni thread confronta la propria threadBestClique con la globalBestClique e, se ha trovato una clique più grande, la aggiorna.
	5.	Ritorno del Risultato:
	•	La funzione restituisce la migliore clique trovata.



Immagina un grafo in cui il vertice 0 è connesso a 1, 2 e 3; il vertice 1 a 0, 2; il vertice 2 a 0, 1; e il vertice 3 solo a 0.
	•	Partendo da 0, inizialmente la clique è {0} e i candidati sono {1,2,3}.
	•	Ordinando i candidati (supponiamo che 1 e 2 abbiano grado maggiore di 3), il thread prova:
	•	Aggiunge 1 → clique diventa {0,1}; aggiorna i candidati all’intersezione dei vicini di 0 e 1 (che diventerà {2} se solo 2 è comune).
	•	Aggiunge 2 → clique diventa {0,1,2}.
	•	3 non viene più considerato perché non appartiene all’intersezione.
	•	Il risultato è una clique di dimensione 3. Parallelamente, altri thread potrebbero partire da altri vertici, ma alla fine viene scelto il più grande.

4. sequentialGreedyCliqueExpansion

Cosa fa:
Implementa l’espansione greedy della clique in modalità sequenziale (senza bitset)
Come funziona:
	1.	Ordinamento dei Vertici:
	•	Ottiene tutti i vertici e li ordina per grado decrescente.
	2.	Per Ogni Vertice:
	•	Inizia una clique con il vertice v.
	•	Estrae i vicini di v e li ordina per grado decrescente.
	•	Per ogni vicino u:
	•	Controlla, iterando su ogni vertice w già nella clique, se esiste un arco tra u e w usando boost::edge(u, w, g).
	•	Se u è adiacente a tutti i vertici in currentClique, lo aggiunge.
	3.	Aggiornamento della Migliore Clique:
	•	Se la clique trovata è più grande di quella migliore fino a quel punto, la sostituisce.
	4.	Ritorno del Risultato:
	•	Ritorna la migliore clique trovata.

 

5. sequentialGreedyCliqueExpansionBitset

Cosa fa:
È una variante del metodo sequenziale che usa i bitset precalcolati (ottenuti con computeNeighborBitsets) per velocizzare il controllo di adiacenza.

Come funziona:
	1.	Ordinamento dei Vertici:
	•	Come prima, ordina i vertici per grado decrescente.
	2.	Per Ogni Vertice:
	•	Inizia una clique con il vertice v.
	•	Estrae e ordina i vicini di v.
	•	Per ogni vicino u:
	•	Per ogni vertice w già nella clique, controlla rapidamente se u è un vicino di w con neighborBitsets[w].test(u).
	•	Se per ogni w il test è positivo, aggiunge u alla clique.
	3.	Aggiornamento della Migliore Clique:
	•	Aggiorna la migliore clique se quella corrente è più grande.
	4.	Ritorno del Risultato:
	•	Restituisce la migliore clique trovata.


L’uso dei bitset rende il test “è u vicino a tutti i vertici della clique?” molto più veloce, perché invece di iterare sugli archi del grafo si esegue un semplice test sul bitset.

Algoritmo di Colorazione del Grafo

6. greedyColoring

Cosa fa:
Esegue una colorazione greedy del grafo, assegnando ad ogni vertice il colore (numero intero) più piccolo non usato dai suoi vicini.

Come funziona:
	1.	Inizializzazione:
	•	Determina il numero di vertici n e inizializza un vettore colors di dimensione n con valore -1 (non colorato).
	2.	Ordinamento dei Vertici:
	•	Costruisce una lista di vertici e li ordina in ordine discendente di grado.
Perché questo ordinamento?
In molti algoritmi greedy, partire dai vertici più connessi aiuta a minimizzare l’uso complessivo dei colori.
	3.	Colorazione per Ogni Vertice:
	•	Per ogni vertice v:
	•	Inizializza un bitset usedColors di dimensione MAX_FIXED_COLORS che terrà traccia dei colori già usati dai vertici adiacenti.
	•	Itera sui vicini di v e, per ogni vicino colorato (cioè con colors[u] != -1), imposta il bit corrispondente al colore di u su true.
	•	Cerca il primo colore (a partire da 0) non usato: incrementa un contatore color finché usedColors.test(color) restituisce true.
	•	Assegna a v il colore trovato.
	4.	Ritorno del Risultato:
	•	Restituisce il vettore colors che associa ad ogni vertice il colore assegnato.

Esempio Visivo:

Considerazioni Finali
	•	Utilizzo dei Bitset:
l’utilizzo dei bitset consente di eseguire operazioni di intersezione e test in tempo costante (o comunque molto veloce) rispetto ai controlli espliciti sugli archi. Questo è particolarmente utile nelle operazioni di espansione della clique, dove si deve verificare ripetutamente se un vertice è adiacente a tutti quelli già selezionati.
	•	Ordinamento per Grado:
L’ordinamento dei vertici (e dei vicini) in ordine decrescente di grado è una strategia euristica che tende a partire dai vertici “più fortemente connessi”, aumentando le probabilità di costruire clique grandi e di ridurre il numero totale di colori usati nella colorazione.
	•	Parallelismo:
L’implementazione parallela (con OpenMP) permette di sfruttare più core del processore, distribuendo la ricerca delle clique tra vari thread. Ogni thread lavora su vertici differenti e, al termine, si sceglie la clique più grande trovata.


 
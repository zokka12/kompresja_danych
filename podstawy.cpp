#include <iostream>
#include <fstream>
#include <map>
#include <string>

using namespace std;
// Węzeł drzewa Huffmana
struct Node {
    char ch;
    int liczba_wystapien;
    Node* left;
    Node* right;
// Konstruktor dla liścia (znak i jego częstotliwość)
    Node(char znak, int czestotliwosc) {
        ch = znak;
        liczba_wystapien = czestotliwosc;
        left = nullptr;
        right = nullptr;
    }
// Konstruktor dla węzła wewnętrznego (częstotliwość i wskaźniki na dzieci)
    Node(int czestotliwosc, Node* l, Node* r) {
        ch = '\0'; // Węzeł wewnętrzny nie przechowuje konkretnego znaku dlatego ma znak pusty
        liczba_wystapien = czestotliwosc; //częstotliwość węzła wewnętrznego to suma częstotliwości jego dzieci
        left = l; // Wskaźnik na lewe dziecko
        right = r; // Wskaźnik na prawe dziecko
    }
};

// Komparator do kolejki priorytetowej 
// Chcemy, aby na górze były węzły o MAKSYMALNIE MAŁEJ częstotliwości
struct Compare {
    // funkcja zwraca true jeżeli liczba wystąpień węzła lewego jest większa niż liczba wystąpień węzła prawego
    bool operator()(Node* l, Node* r) {
        return l->liczba_wystapien > r->liczba_wystapien;
    }
};
// Funkcja do zbierania wystąpień znaków z pliku
map<char, int> zbierz_wystapienia(const string& nazwa_pliku) {
    // Otwieramy plik w trybie binarnym
    ifstream inFile(nazwa_pliku, ios::binary);
    map<char, int> liczba_wystapien;
    char ch;

    while (inFile.get(ch)) {
        liczba_wystapien[ch]++;
    }

    inFile.close();
    return liczba_wystapien;
}
// jak zapisywac słownik w pliku wejsciowym, napewno nie dla wszystkich możliwych znakow, tylko dla znaków które występują w tekście
// a 110
// b 01
// c 1101
// d 10
// licznik - można zapisać slownik w sposób: jeden bajt licznika czyli 8 bitów
//00000100
//licznik 8b
// licznik 8b 32 to 5 b
//67
//licznik nie trzeba zapisywać jako całego drzewa - można zapisać znak i słowo kodowe i jaka długość - literka w ascii, rozmiar i same bity
//zapisywanie ios::binary iostream -> specyfikacja binary
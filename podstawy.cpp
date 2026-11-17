#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <queue>
#include <vector>

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

Node* zbuduj_drzewo(const map<char, int>& statystyki) {
    // Tworzymy kolejkę priorytetową (kopiec minimalny)
    // (typ przechowywanych elementów, kontener, komparator(żeby najmniejsze elementy były na górze))
    priority_queue<Node*, vector<Node*>, Compare> kolejka;

    // Dla każdej pary (znak - liczba wystąpień) z mapy tworzymy nowy liść
    // auto -  automatycznie rozpoznaje typ zmiennej na podstawie przypisanej wartości
    //statystyki - zmienna (kontener), przez którą pętla ma przejść
    //Dla każdej (for) pojedynczej rzeczy (para), której nie będę modyfikować (const)
    //ani kopiować (&), a jej typ niech C++ zgadnie sam (auto), wyciągniętej ze zbioru
    //(:) statystyki – zrób to, co jest w klamrach.
    for (const auto& para : statystyki) {
        char znak = para.first; // wyciągamy znak z pliku
        int czestotliwosc = para.second; // wyciągamy częstotliwość wystąpień znaku z pliku
        
        // Używamy Twojego pierwszego konstruktora dla liści
        kolejka.push(new Node(znak, czestotliwosc));
    }

    //Łączymy węzły, dopóki w kolejce nie zostanie tylko korzeń drzewa
    while (kolejka.size() > 1) {
        // Zdejmujemy dwa drzewa o najmniejszej liczbie wystąpień
        Node* lewe_dziecko = kolejka.top();
        kolejka.pop();

        Node* prawe_dziecko = kolejka.top();
        kolejka.pop();

        // Obliczamy sumę wystąpień z obu zdjętych węzłów
        int suma = lewe_dziecko->liczba_wystapien + prawe_dziecko->liczba_wystapien;

        // Tworzymy węzeł wewnętrzny używając Twojego drugiego konstruktora
        Node* rodzic = new Node(suma, lewe_dziecko, prawe_dziecko);

        // Wstawiamy połączone drzewo z powrotem do kolejki
        kolejka.push(rodzic);
    }

    // Ostatni pozostały w kolejce element to korzeń gotowego drzewa Huffmana
    return kolejka.top();
}

//  Generowanie kodów bitowych (przejście po drzewie w głąb - DFS)
void generuj_kody(Node* korzen, string obecny_kod, map<char, string>& slownik_kodow) {
    //Zabezpieczenie: jeśli węzeł jest pusty, przerywamy działanie dla tej ścieżki
    if (korzen == nullptr) {
        return;
    }

    //Sprawdzenie, czy dotarliśmy do liścia (liść nie ma lewego ani prawego dziecka)
    if (korzen->left == nullptr && korzen->right == nullptr) {
        // Zabezpieczenie dla pliku zawierającego tylko 1 rodzaj znaku (np. "aaaaa")
        if (obecny_kod == "") {
            slownik_kodow[korzen->ch] = "0";
        } else {
            // Zapisujemy wygenerowany ciąg bitów pod kluczem konkretnego znaku
            slownik_kodow[korzen->ch] = obecny_kod;
        }
        return;
    }
    //Zejście rekurencyjne w dół drzewa:
    // Idąc w lewo dopisujemy '0', idąc w prawo dopisujemy '1'
    generuj_kody(korzen->left, obecny_kod + "0", slownik_kodow);
    generuj_kody(korzen->right, obecny_kod + "1", slownik_kodow);
}

// Zapis słownika (nagłówka) do pliku binarnego
void zapisz_slownik(ofstream& outFile, const map<char, string>& slownik_kodow) {
    // Zapisujemy liczbę unikalnych znaków na 1 bajcie (unsigned char: 0-255)
    // Uwaga: jeśli wystąpią wszystkie 256 znaków ASCII, 256 zapisze się jako 0 (obsłużymy to przy odczycie)
    unsigned char liczba_znakow = static_cast<unsigned char>(slownik_kodow.size());
    outFile.write(reinterpret_cast<const char*>(&liczba_znakow), sizeof(liczba_znakow));

    // Iterujemy po każdej parze (znak -> ciąg bitów) w naszym słowniku
    for (const auto& para : slownik_kodow) {
        char znak = para.first;
        const string& kod = para.second;
        unsigned char dlugosc_kodu = static_cast<unsigned char>(kod.length());

        // Zapisujemy 1 bajt znaku oraz 1 bajt określający długość jego kodu w bitach
        outFile.write(&znak, sizeof(znak));
        outFile.write(reinterpret_cast<const char*>(&dlugosc_kodu), sizeof(dlugosc_kodu));

        // Pakujemy tekstowy ciąg zer i jedynek (np. "110") w prawdziwe bity
        // Obliczamy, ile pełnych bajtów potrzeba na zapisanie 'dlugosc_kodu' bitów (zaokrąglenie w górę)
        int potrzebne_bajty = (dlugosc_kodu + 7) / 8;
        vector<unsigned char> bajty_kodu(potrzebne_bajty, 0); // Wektor wypełniony zerami

        for (size_t i = 0; i < dlugosc_kodu; ++i) {
            if (kod[i] == '1') {
                // Ustawiamy odpowiedni bit na 1 (od najbardziej znaczącego bitu w bajcie, czyli od lewej)
                int indeks_bajtu = i / 8;
                int pozycja_bitu = 7 - (i % 8);
                bajty_kodu[indeks_bajtu] |= (1 << pozycja_bitu);
            }
        }

        // Zapisujemy spakowane bajty kodu do pliku
        outFile.write(reinterpret_cast<const char*>(bajty_kodu.data()), potrzebne_bajty);
    }
}
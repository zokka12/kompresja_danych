#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <cstdint>

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
    bool operator()(const Node* l, const Node* r) const{
        return l->liczba_wystapien > r->liczba_wystapien;
    }
};

void usun_drzewo(Node* korzen) {
    if (korzen == nullptr) {
        return;
    }
    usun_drzewo(korzen->left);
    usun_drzewo(korzen->right);
    delete korzen;
}
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

Node* zbuduj_drzewo(const map<char, int>& statystyki) {
    // Tworzymy kolejkę priorytetową  (typ przechowywanych elementów, kontener - elementy sa w vector, komparator(żeby najmniejsze elementy były na górze))
    priority_queue<Node*, vector<Node*>, Compare> kolejka;

    // Dla każdej pary (znak - liczba wystąpień) z mapy tworzymy nowy liść
    // auto -  automatycznie rozpoznaje typ zmiennej na podstawie przypisanej wartości
    //statystyki - zmienna, przez którą pętla ma przejść
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
//  Generowanie kodów bitowych 
void generuj_kody(Node* korzen, string obecny_kod, map<char, string>& slownik_kodow) {
    //Zabezpieczenie: jeśli węzeł jest pusty, przerywamy działanie dla tej ścieżki
    if (korzen == nullptr) {
        return;
    }
    //Sprawdzenie, czy dotarliśmy do liścia , liscie nie maja dzieci
    if (korzen->left == nullptr && korzen->right == nullptr) {
        // Zabezpieczenie dla pliku zawierającego tylko jedenego rodzaj znaku
        if (obecny_kod == "") {
            slownik_kodow[korzen->ch] = "0";
        } else {
            // Zapisujemy wygenerowany ciąg bitów pod kluczem konkretnego znaku
            slownik_kodow[korzen->ch] = obecny_kod;
        }
        return;
    }
    //Zejście rekurencyjne w dół drzewa - idąc w lewo dopisujemy '0', idąc w prawo dopisujemy '1'
    generuj_kody(korzen->left, obecny_kod + "0", slownik_kodow);
    generuj_kody(korzen->right, obecny_kod + "1", slownik_kodow);
}

// Zapis słownika (nagłówka) do pliku binarnego
void zapisz_slownik(ofstream& outFile, const map<char, string>& slownik_kodow) {
    // Zapisujemy liczbę unikalnych znaków na 1 bajcie 
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
// Funkcja obliczająca rozmiary i procent zaoszczędzonego miejsca
void wyswietl_statystyki(uint64_t rozmiar_przed, uint64_t rozmiar_naglowka, uint64_t rozmiar_po) {
    cout << "kompresja zakonczona sukcesem" << endl;
    if (rozmiar_przed > 0) {
        double zaoszczedzono = (static_cast<double>(rozmiar_przed) - static_cast<double>(rozmiar_po)) / rozmiar_przed * 100.0;
        cout << "Rozmiar: z " << rozmiar_przed << " B na " << rozmiar_po << " B "
             << "(naglowek: " << rozmiar_naglowka << " B, zaoszczedzono: " << zaoszczedzono << "%)" << endl;
    } else {
        cout << "Rozmiar: z 0 B na 0 B (pusty plik)" << endl;
    }
}

//Główna funkcja realizująca proces kompresji
void kompresuj_plik(const string& plik_wejsciowy, const string& plik_wyjsciowy) {
    ifstream testPliku(plik_wejsciowy, ios::binary);
    if (!testPliku.is_open()) {
        cout << "nie mozna otworzyc pliku wejsciowego" << endl;
        return;
    }
    testPliku.close();
    // Zbieramy statystyki 
    map<char, int> statystyki = zbierz_wystapienia(plik_wejsciowy);
    if (statystyki.empty()) {
        ofstream pustyOut(plik_wyjsciowy, ios::binary);
        pustyOut.close();
        cout << "plik wejsciowy jest pusty, utworzono plik wyjsciowy" << endl;
        return;
    }

    // Obliczamy całkowitą liczbę znaków w pliku wejściowym 
    uint32_t calkowita_liczba_znakow = 0;
    for (const auto& para : statystyki) {
        calkowita_liczba_znakow += para.second;
    }

    // Budujemy drzewo Huffmana 
    Node* korzen = zbuduj_drzewo(statystyki);

    // Generujemy słownik kodów bitowych 
    map<char, string> slownik_kodow;
    generuj_kody(korzen, "", slownik_kodow);
    usun_drzewo(korzen);

    // Otwieramy plik wejściowy do ponownego odczytu oraz plik wyjściowy do zapisu binarnego
    ifstream inFile(plik_wejsciowy, ios::binary);
    ofstream outFile(plik_wyjsciowy, ios::binary);
    if (!outFile.is_open()) {
        cout << "nie mozna utworzyc pliku wyjsciowego" << endl;
        return;
    }
    uint64_t rozmiar_naglowka = 1 + sizeof(calkowita_liczba_znakow);
    for (const auto& para : slownik_kodow) {
        rozmiar_naglowka += 2 + (para.second.length() + 7) / 8;
    }
    // Zapisujemy słownik w nagłówku
    zapisz_slownik(outFile, slownik_kodow);
    // Zapisujemy całkowitą liczbę znaków (4 bajty), aby dekompresor wiedział kiedy przestać czytać bity
    outFile.write(reinterpret_cast<const char*>(&calkowita_liczba_znakow), sizeof(calkowita_liczba_znakow));
    
    // Właściwe kodowanie treści pliku bit po bicie
    uint64_t rozmiar_po = rozmiar_naglowka;
    unsigned char bufor_bitowy = 0; 
    int licznik_bitow = 0;          
    char ch;

    while (inFile.get(ch)) {
        // Pobieramy ciąg zer i jedynek dla aktualnego znaku ze słownika
        const string& kod = slownik_kodow[ch];
        for (char bit : kod) {
            // Przesuwamy istniejące bity w buforze o 1 pozycję w lewo, robiąc miejsce z prawej strony
            bufor_bitowy = (bufor_bitowy << 1);
            // Jeśli aktualny znak kodu to '1', wpisujemy jedynkę na zwolnione miejsce z prawej strony
            if (bit == '1') {
                bufor_bitowy = bufor_bitowy | 1;
            }
            licznik_bitow++;
            // Gdy bufor uzbiera pełny bajt (8 bitów), zapisujemy go do pliku i zerujemy liczniki
            if (licznik_bitow == 8) {
                outFile.put(bufor_bitowy);
                rozmiar_po++;
                bufor_bitowy = 0;
                licznik_bitow = 0;
            }
        }
    }

    // Obsługa niedopełnionego ostatniego bajtu (Padding)
    // Jeśli po przeczytaniu całego pliku w buforze zostały jakieś bity (np. 3 bity)
    if (licznik_bitow > 0) {
        // Przesuwamy je maksymalnie w lewo, dopisując z prawej strony puste zera do pełnych 8 bitów
        bufor_bitowy = bufor_bitowy << (8 - licznik_bitow);
        outFile.put(bufor_bitowy);
        rozmiar_po++;
    }
    //wymusza natychmiastowy zapis danych z tymczasowego bufora w pamięci RAM na fizyczny dysk
    outFile.flush();
    bool zapis_udany = outFile.good();

    inFile.close();
    outFile.close();

    if (zapis_udany) {
        wyswietl_statystyki(calkowita_liczba_znakow, rozmiar_naglowka, rozmiar_po);
    } else {
        cout << "blad podczas zapisu do pliku" << endl;
    }
}

// Główna funkcja realizująca proces dekompresji
void dekompresuj_plik(const string& plik_skompresowany, const string& plik_wyjsciowy) {
    ifstream inFile(plik_skompresowany, ios::binary);
    if (!inFile.is_open()) {
        cout << "nie mozna otworzyc pliku do dekompresji" << endl;
        return;
    }
    // Odczytujemy liczbę unikalnych znaków ze słownika (1 bajt)
    unsigned char liczba_znakow_bajt;
    inFile.read(reinterpret_cast<char*>(&liczba_znakow_bajt), sizeof(liczba_znakow_bajt));
    // Jeśli plik był pusty lub uszkodzony
    // Jeśli plik był pusty (0 bajtów)
    if (inFile.eof()) {
        ofstream pustyOut(plik_wyjsciowy, ios::binary);
        pustyOut.close();
        cout << "plik skompresowany jest pusty, utworzono pusty plik wyjsciowy" << endl;
        return;
    }
    int liczba_znakow = (liczba_znakow_bajt == 0) ? 256 : liczba_znakow_bajt;
    // Odwrócony słownik: ciąg bitów (np. "110") -> konkretny znak (np. 'a')
    
    map<string, char> odwr_slownik;
    // Odczytujemy kolejne wpisy słownika
    for (int k = 0; k < liczba_znakow; ++k) {
        char znak;
        unsigned char dlugosc_kodu;

        inFile.read(&znak, sizeof(znak));
        inFile.read(reinterpret_cast<char*>(&dlugosc_kodu), sizeof(dlugosc_kodu));

        // Obliczamy, ile bajtów zajmują spakowane bity tego kodu
        int potrzebne_bajty = (dlugosc_kodu + 7) / 8;
        vector<unsigned char> bajty_kodu(potrzebne_bajty);
        inFile.read(reinterpret_cast<char*>(bajty_kodu.data()), potrzebne_bajty);

        // Rozpakowujemy bity z powrotem do tekstowego ciągu zer i jedynek
        string kod = "";
        for (int i = 0; i < dlugosc_kodu; ++i) {
            int indeks_bajtu = i / 8;
            int pozycja_bitu = 7 - (i % 8);
            
            // Sprawdzamy, czy na danej pozycji w bajcie znajduje się 1 czy 0
            bool czy_jedynka = (bajty_kodu[indeks_bajtu] & (1 << pozycja_bitu)) != 0;
            kod += czy_jedynka ? '1' : '0';
        }

        odwr_slownik[kod] = znak;
    }
    // Odczytujemy całkowitą liczbę znaków oryginalnego tekstu (4 bajty)
    uint32_t calkowita_liczba_znakow = 0;
    inFile.read(reinterpret_cast<char*>(&calkowita_liczba_znakow), sizeof(calkowita_liczba_znakow));
    // Otwieramy plik wyjściowy w trybie binarnym (żeby zachować dokładne znaki nowej linii)
    ofstream outFile(plik_wyjsciowy, ios::binary);
    // Dekodowanie głównej treści bit po bicie
    string aktualny_kod = "";
    uint32_t odkodowane_znaki = 0;
    char bajt;
    // Czytamy plik bajt po bajcie, dopóki nie odkodujemy wszystkich oczekiwanych znaków
    while (odkodowane_znaki < calkowita_liczba_znakow && inFile.get(bajt)) {
        unsigned char u_bajt = static_cast<unsigned char>(bajt);

        // Rozbijamy pojedynczy bajt na 8 osobnych bitów (od lewej do prawej, czyli od 7 do 0)
        for (int i = 7; i >= 0; --i) {
            // Wyciągamy i-ty bit z bajtu
            bool bit = (u_bajt & (1 << i)) != 0;
            aktualny_kod += bit ? '1' : '0';

            // Sprawdzamy, czy zebrany ciąg bitów istnieje w naszym słowniku
            // Ponieważ kody Huffmana są prefiksowe, dopasowanie jest zawsze jednoznaczne
            auto it = odwr_slownik.find(aktualny_kod);
            if (it != odwr_slownik.end()) {
                // Znaleziono literę Zapisujemy ją do pliku wyjściowego
                outFile.put(it->second);
                
                // Resetujemy bufor kodu dla kolejnej litery i zwiększamy licznik
                aktualny_kod = "";
                odkodowane_znaki++;

                // Jeśli odkodowaliśmy już cały tekst, przerywamy pętlę (ignorujemy zera dopełniające ostatni bajt)
                if (odkodowane_znaki == calkowita_liczba_znakow) {
                    break;
                }
            }
        }
    }

    inFile.close();
    outFile.close();
    cout << "dekompresja zakonczona sukcesem" << endl;
}

// Główna funkcja programu obsługująca argumenty z linii poleceń
int main(int argc, char* argv[]) {
    string tryb, plik_wejsciowy, plik_wyjsciowy;

    if (argc == 4) {
        tryb = argv[1];
        plik_wejsciowy = argv[2];
        plik_wyjsciowy = argv[3];
    } 
    else if (argc == 1) {
        cout << "Tryb (k - kompresja, d - dekompresja): ";
        cin >> tryb;
        cout << "Plik wejsciowy: ";
        cin >> plik_wejsciowy;
        cout << "Plik wyjsciowy: ";
        cin >> plik_wyjsciowy;
    } 
    else {
        cout << "Blad Nieprawidlowa liczba argumentow." << endl;
        cout << "Uzycie: " << argv[0] << " -k|-d <plik_wejsciowy> <plik_wyjsciowy>" << endl;
        return 1;
    }

    if (tryb == "-k" || tryb == "k") {
        kompresuj_plik(plik_wejsciowy, plik_wyjsciowy);
    } 
    else if (tryb == "-d" || tryb == "d") {
        dekompresuj_plik(plik_wejsciowy, plik_wyjsciowy);
    } 
    else {
        cout << "Nieznany tryb: " << tryb << ". Uzyj k (kompresja) lub d (dekompresja)." << endl;
        return 1;
    }

    return 0;
}
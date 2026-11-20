#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <cstdint>

using namespace std;
// wezeł drzewa huffmana
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
// funkcja do zbierania wystąpień znaków z pliku
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
    // tworzymy kolejkę priorytetową  (typ przechowywanych elementów, kontener - elementy sa w vector, komparator(żeby najmniejsze elementy były na górze))
    priority_queue<Node*, vector<Node*>, Compare> kolejka;
    
    for (const auto& para : statystyki) {
        char znak = para.first; // wyciągamy znak z pliku
        int czestotliwosc = para.second; // wyciągamy częstotliwość wystąpień znaku z pliku
        // tworzymy nowy węzeł liścia dla każdego znaku i jego częstotliwości, a następnie dodajemy go do kolejki priorytetowej
        kolejka.push(new Node(znak, czestotliwosc));
    }
    //łaczenie wezłów do momentu aż powstanie jeden wezel
    while (kolejka.size() > 1) {
        // zdejmuje dwa drzewa o najmniejszej liczbie wystąpień
        Node* lewe_dziecko = kolejka.top();
        kolejka.pop();
        Node* prawe_dziecko = kolejka.top();
        kolejka.pop();
        // oblicza sumę wystąpień z obu zdjętych węzłów
        int suma = lewe_dziecko->liczba_wystapien + prawe_dziecko->liczba_wystapien;
        // tworzymy węzeł wewnętrzny używając sumy jako liczby wystąpień i ustawi zdjęte węzły jako jego dzieci
        Node* rodzic = new Node(suma, lewe_dziecko, prawe_dziecko);
        // wstawiamy podłączone drzewo spowrotem do kolejki
        kolejka.push(rodzic);
    }
    // zwraca ostatni element pozostały czyli drzewo huffmana
    return kolejka.top();
}

//  GENEROWANIE KODÓW BITOWYCH
void generuj_kody(Node* korzen, string obecny_kod, map<char, string>& slownik_kodow) {
    //jesli korzen jest pusty to zwracamy bo nie ma drzewa do przeszukania
    if (korzen == nullptr) {
        return;
    }
    //czy dorarlismy do liscia , czyli wezła bez dzieci
    if (korzen->left == nullptr && korzen->right == nullptr) {
        // jesli plik składał się z jednego znaku, to przypisujemy mu kod 0
        if (obecny_kod == "") {
            slownik_kodow[korzen->ch] = "0";
        } else {
            // zapisujemy kod bitowy dla danego znaku w słowniku
            slownik_kodow[korzen->ch] = obecny_kod;
        }
        return;
    }
    //zejście rekurencyjne w dół drzewa - idąc w lewo dopisujemy 0, idąc w prawo dopisujemy 1
    // -> przejdz pod adress korzen i odczytaj jego pole left
    generuj_kody(korzen->left, obecny_kod + "0", slownik_kodow);
    generuj_kody(korzen->right, obecny_kod + "1", slownik_kodow);
}

// ZAPIS SŁOWNIKA - NAGŁÓWKA DO PLIKU BINARNEGO
void zapisz_slownik(ofstream& outFile, const map<char, string>& slownik_kodow) {
    // zapisujemy liczbe - unikalnych znaków size() - na jednym bajcie - unsighned char
    // static cast - sprawia ze kom nie rzuca wyjatkeim
    unsigned char liczba_znakow = static_cast<unsigned char>(slownik_kodow.size());
    // zapisujemy jeden bajt do pliku, reinterpretując unsigned char jako const char* - wskaźnik do danych
    // sizeof(liczba_znakow) - odpowiada za to że fun .write wie na jakiej wielkości zapisać dane do pliku
    outFile.write(reinterpret_cast<const char*>(&liczba_znakow), sizeof(liczba_znakow));
    // iteracja przez wszystkie pozycje w słóowniku, const auto& para odpowiada za to że nie kopiujemy danych tylko odwołujemy się do nich
    for (const auto& para : slownik_kodow) {
        // zapisujemy konkretny znak
        char znak = para.first;
        // zapisujemy kod huffmana
        const string& kod = para.second;
        // zapisujemy długość kodu w bitach na 1jednym bicie
        unsigned char dlugosc_kodu = static_cast<unsigned char>(kod.length());
        // zapisujemy jeden bajt z sama litera
        outFile.write(&znak, sizeof(znak));
        // zapisujemy jeden bajt z długością kodu w bitach
        // dzięki temu dekompresor wie ile bitów odczytać dla danego znaku
        outFile.write(reinterpret_cast<const char*>(&dlugosc_kodu), sizeof(dlugosc_kodu));
    
        int potrzebne_bajty = (dlugosc_kodu + 7) / 8;
        // tworzymy wektor o wyliczonej liczbie bajtów i wypełniamy go zerami, aby przygotować miejsce na spakowane bity kodu
        vector<unsigned char> bajty_kodu(potrzebne_bajty, 0); 

        for (size_t i = 0; i < dlugosc_kodu; ++i) {
            if (kod[i] == '1') {
                //sprawdzamy do ktorego bita trafiamy
                int indeks_bajtu = i / 8;
                int pozycja_bitu = 7 - (i % 8);
                // ustawiamy odpowiedni bit na 1
                bajty_kodu[indeks_bajtu] |= (1 << pozycja_bitu);
            }
        }
        // zapisujemy spakowane bity kodu do pliku binarnego z  potrzebną liczbą bajtów
        outFile.write(reinterpret_cast<const char*>(bajty_kodu.data()), potrzebne_bajty);
    }
}

// ile bajtów miał plik przed kompresja
//ile bitów będzie miał słownik i licznik znaków
//ile bajtów będzie miał plik po kompresji
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

//główna funkcja realizująca proces kompresji
void kompresuj_plik(const string& plik_wejsciowy, const string& plik_wyjsciowy) {
    //otwieramy plik wejściowy w trybie binarnym, aby sprawdzić czy istnieje i jest dostępny do odczytu
    ifstream testPliku(plik_wejsciowy, ios::binary);
    if (!testPliku.is_open()) {
        cout << "nie mozna otworzyc pliku wejsciowego" << endl;
        return;
    }
    testPliku.close();
    // zwracamy tabelkę, przy każdym znaku jest liczna wystąpień
    map<char, int> statystyki = zbierz_wystapienia(plik_wejsciowy);
    if (statystyki.empty()) {
        ofstream pustyOut(plik_wyjsciowy, ios::binary);
        pustyOut.close();
        cout << "plik wejsciowy jest pusty, utworzono plik wyjsciowy" << endl;
        return;
    }

    // sumuje wszystkie wystąpienia znakówile dokładnei miał plik wejściowy
    uint32_t calkowita_liczba_znakow = 0;
    for (const auto& para : statystyki) {
        calkowita_liczba_znakow += para.second;
    }
    // budujemy drzewo Huffmana 
    // zwraca wskaźnik na korzeń drzewa
    Node* korzen = zbuduj_drzewo(statystyki);
    // generujemy słownik kodów bitowych 
    map<char, string> slownik_kodow;
    generuj_kody(korzen, "", slownik_kodow);
    // usuwamy drzewo huffmana, aby zwolnić pamięć
    usun_drzewo(korzen);

    // otwieramy plik wejściowy do ponownego odczytu oraz plik wyjściowy do zapisu binarnego
    ifstream inFile(plik_wejsciowy, ios::binary);
    ofstream outFile(plik_wyjsciowy, ios::binary);
    if (!outFile.is_open()) {
        cout << "nie mozna utworzyc pliku wyjsciowego" << endl;
        return;
    }
    // obliczamy rozmiar nagłówka w bajtach: jeden bajt na liczbę unikalnych znaków + drugi bajty na każdy wpis słownika + spakowane bity kodów
    uint64_t rozmiar_naglowka = 1 + sizeof(calkowita_liczba_znakow);
    for (const auto& para : slownik_kodow) {
        rozmiar_naglowka += 2 + (para.second.length() + 7) / 8;
    }
    // zapisujemy słownik do pliku binarnego
    zapisz_slownik(outFile, slownik_kodow);
    // zapisujemy całkowitą liczbę znaków oryginalnego tekstu, dla dekompresora
    outFile.write(reinterpret_cast<const char*>(&calkowita_liczba_znakow), sizeof(calkowita_liczba_znakow));
    
    // rozmiar pliku po kompresji zaczyna się od rozmiaru nagłówka
    uint64_t rozmiar_po = rozmiar_naglowka;
    // bufor bitowy do przechowywania spakowanych bitów przed zapisaniem ich do pliku
    unsigned char bufor_bitowy = 0; 
    // licznik bitów w buforze, aby wiedzieć kiedy mamy pełny bajt do zapisania
    int licznik_bitow = 0;          
    char ch;
    // czytamy po jednym znaku z pliku wejściowego aż do końca
    while (inFile.get(ch)) {
        // bierzemy przypisany kod huffmana
        const string& kod = slownik_kodow[ch];
        // iterujemy przez każdy bit w kodzie
        for (char bit : kod) {
            // przesuwamy miejsce w buforze o jeden w lewo, aby zrobić miejsce na nowy bit z prawej strony
            bufor_bitowy = (bufor_bitowy << 1);
            // jesli bit == 1 to zamieniamy ostatni bit bufora na 1, w przeciwnym razie pozostaje 0
            if (bit == '1') {
                bufor_bitowy = bufor_bitowy | 1;
            }
            licznik_bitow++;
            // gdy buffor uzbiera 8 bitów
            if (licznik_bitow == 8) {
                // zapisujemy pełny bajt do pliku wyjściowego
                outFile.put(bufor_bitowy);
                // zwiększamy licznik rozmiaru pliku po kompresji o 1 bajt
                rozmiar_po++;
                bufor_bitowy = 0;
                licznik_bitow = 0;
            }
        }
    }

    // obsługa niedopełnionego ostatniego bajtu
    // jeśli po przeczytaniu całego pliku w buforze zostały jakieś bity (
    if (licznik_bitow > 0) {
        // przesuwamy je maksymalnie w lewo, dopisując z prawej strony puste zera 
        bufor_bitowy = bufor_bitowy << (8 - licznik_bitow);
        outFile.put(bufor_bitowy);
        rozmiar_po++;
    }
    // wymusza natychmiastowy zapis danych z pamieci na dysk
    outFile.flush();
    // sprawdza błędy
    bool zapis_udany = outFile.good();

    inFile.close();
    outFile.close();

    if (zapis_udany) {
        wyswietl_statystyki(calkowita_liczba_znakow, rozmiar_naglowka, rozmiar_po);
    } else {
        cout << "blad podczas zapisu do pliku" << endl;
    }
}

// główna funkcja realizująca proces dekompresji
void dekompresuj_plik(const string& plik_skompresowany, const string& plik_wyjsciowy) {
    //otwieramy plik zeby czytać surowe bajty bez zmian
    ifstream inFile(plik_skompresowany, ios::binary);
    if (!inFile.is_open()) {
        cout << "nie mozna otworzyc pliku do dekompresji" << endl;
        return;
    }
    // odczytujemy liczbę unikalnych znaków ze słownika 
    unsigned char liczba_znakow_bajt;
    // próbujemy wczytać pierwszy baji z brzegu, który mowi ile unikalnych znaków jest w słowniku
    inFile.read(reinterpret_cast<char*>(&liczba_znakow_bajt), sizeof(liczba_znakow_bajt));
    // jeśli plik był pusty lub uszkodzony
    if (inFile.eof()) {
        ofstream pustyOut(plik_wyjsciowy, ios::binary);
        pustyOut.close();
        cout << "plik skompresowany jest pusty, utworzono pusty plik wyjsciowy" << endl;
        return;
    }
    //zabezpieczenie przed wystapieniem wszystkich 256 znaków
    int liczba_znakow = (liczba_znakow_bajt == 0) ? 256 : liczba_znakow_bajt;
    // tworzymy owrócony słownik
    map<string, char> odwr_slownik;
    // odczytujemy kolejne wpisy słownika
    for (int k = 0; k < liczba_znakow; ++k) {
        char znak;
        unsigned char dlugosc_kodu;

        inFile.read(&znak, sizeof(znak));
        inFile.read(reinterpret_cast<char*>(&dlugosc_kodu), sizeof(dlugosc_kodu));

        // obliczamy, ile bajtów zajmują spakowane bity tego kodu
        int potrzebne_bajty = (dlugosc_kodu + 7) / 8;
        vector<unsigned char> bajty_kodu(potrzebne_bajty);
        inFile.read(reinterpret_cast<char*>(bajty_kodu.data()), potrzebne_bajty);

        // rozpakowujemy bity z powrotem do tekstowego ciągu zer i jedynek
        string kod = "";
        for (int i = 0; i < dlugosc_kodu; ++i) {
            int indeks_bajtu = i / 8;
            int pozycja_bitu = 7 - (i % 8);
            
            // sprawdzamy, czy na danej pozycji w bajcie znajduje się 1 czy 0
            bool czy_jedynka = (bajty_kodu[indeks_bajtu] & (1 << pozycja_bitu)) != 0;
            kod += czy_jedynka ? '1' : '0';
        }

        odwr_slownik[kod] = znak;
    }
    // odczytujemy całkowitą liczbę znaków oryginalnego tekstu (4 bajty)
    uint32_t calkowita_liczba_znakow = 0;
    inFile.read(reinterpret_cast<char*>(&calkowita_liczba_znakow), sizeof(calkowita_liczba_znakow));
    // otwieramy plik wyjściowy w trybie binarnym (żeby zachować dokładne znaki nowej linii)
    ofstream outFile(plik_wyjsciowy, ios::binary);
    // dekodowanie głównej treści bit po bicie
    string aktualny_kod = "";
    uint32_t odkodowane_znaki = 0;
    char bajt;
    // czytamy plik bajt po bajcie, dopóki nie odkodujemy wszystkich oczekiwanych znaków
    while (odkodowane_znaki < calkowita_liczba_znakow && inFile.get(bajt)) {
        unsigned char u_bajt = static_cast<unsigned char>(bajt);

        for (int i = 7; i >= 0; --i) {
            // Wyciągamy i-ty bit z bajtu
            bool bit = (u_bajt & (1 << i)) != 0;
            aktualny_kod += bit ? '1' : '0';

            // sprawdzamy czy zebrany ciąg bitów istnieje w naszym słowniku
            auto it = odwr_slownik.find(aktualny_kod);
            if (it != odwr_slownik.end()) {
                // znaleziono literę Zapisujemy ją do pliku wyjściowego
                outFile.put(it->second);
                
                // resetujemy bufor kodu dla kolejnej litery i zwiększamy licznik
                aktualny_kod = "";
                odkodowane_znaki++;

                // jeśli odkodowaliśmy już cały tekst, przerywamy pętlę 
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
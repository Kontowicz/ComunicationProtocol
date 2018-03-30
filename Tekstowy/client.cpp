#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <chrono>
#include <array>
#include <sstream>
#include <conio.h>
#include <clocale>
#include <map>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015
#define IPADDR "127.0.0.1"

char recvbuf[DEFAULT_BUFLEN];
int iResult;
int recvbuflen = DEFAULT_BUFLEN;
/*
KODY OPERACJI
op_0 (¿¹danie przydzielenia ID,
op_1 (suma liczb z pól A i B),
op_2 (róŸnica liczb z pól A i B),
op_3 (mno¿enie liczb z pól A i B),
op_4 (dzielenie liczb z pól A i B),
op_5 (potgowanie liczb z pola A^B),
op_6 (pierwiastkowanie pierwiastek stopnia A z liczby B),
op_7 (logarytm o podstawie A i liczbie logarytmowanej B),
op_8 (porównanie liczb z pol A i B),
op_9 (silnia z liczby z pola A),
op_10 (kod oznaczaj¹cy odpowiedŸ serwera))
op_11 (kod oznaczakoñczenie po³¹czenia)
op_12 (kod oznaczajacy ¿e operacja przeprowadzona przez serwer zakoñczy³a siê b³êdem)
op_13 (zwrot wyniku)

STRUKTURA ODCZYTYWANYCH PAKIETÓW
doZwrotu[0] - czas
doZwrotu[1] - operacja
doZwrotu[2] - status
doZwrotu[3] - id
doZwrotu[4] - argument 1
doZwrotu[5] - argument
*/



std::string floatToString(float liczba)
{
	std::stringstream floatToStr;
	std::string str;
	floatToStr << liczba;
	floatToStr >> str;
	floatToStr.clear();
	return str;
}

std::string intToString(int a)
{
	std::string s;
	sprintf((char*)s.c_str(), "%d", a);
	return s.c_str();
}

void wyslij(SOCKET&ConnectSocket, std::string pakiet)
{
	int iResult;
	iResult = send(ConnectSocket, pakiet.c_str(), pakiet.length(), 0);
	if (iResult == SOCKET_ERROR) {
		std::clog << "send blad: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

std::string odbierz(SOCKET&ConnectSocket) // KOMENTARZE
{
	int iResult;
	std::string dane = "";
	iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);

	if (iResult > 0) {
		dane = "";
		for (int i = 0; i < iResult; i++)
		{
			dane += recvbuf[i];
		}
	}
	else if (iResult == 0)
		std::clog << "Polaczenie zamkniete.\n";
	else
		std::clog << "recv zakonczone bledem: " << WSAGetLastError() << "\n";

	return dane;
}

#pragma region operacje na pakietach
std::string stworzPakiet(std::string operacja, std::string status, std::string id, std::string argument1, std::string argument2)
{
	auto czas = std::chrono::system_clock::now();
	time_t czasAktualny = std::chrono::system_clock::to_time_t(czas);
	std::string czasS = ctime(&czasAktualny);
	czasS.erase(czasS.end() - 1);

	std::string pakiet = "";

	pakiet = "#1:C=" + czasS + "+#2:O=" + operacja + "+#3:S=" + status
		+ "+#4:I=" + id + "+#5:A=" + argument1 + "+#6:B=" + argument2;

	return pakiet;
}

std::array<std::string, 6> dekodujPakiet(std::string pakiet)
{
	/*doZwrotu[0] - czas
	doZwrotu[1] - operacja
	doZwrotu[2] - status
	doZwrotu[3] - id
	doZwrotu[4] - argument 1
	doZwrotu[5] - argument 2*/

	std::array<std::string, 6> doZwrotu;

	int a = pakiet.find("#1:C=") + 5;
	int b = pakiet.find("+#2:O=");

	doZwrotu[0] = pakiet.substr(a, b - a); // czas

	a = pakiet.find("+#2:O=") + 6;
	b = pakiet.find("+#3:S=");
	doZwrotu[1] = pakiet.substr(a, b - a); // operacja

	a = pakiet.find("+#3:S=") + 6;
	b = pakiet.find("+#4:I=");
	doZwrotu[2] = pakiet.substr(a, b - a).c_str(); // status

	a = pakiet.find("+#4:I=") + 6;
	b = pakiet.find("+#5:A=");
	doZwrotu[3] = pakiet.substr(a, b - a); // id

	a = pakiet.find("+#4:I=") + 6;
	b = pakiet.find("+#5:A=");
	doZwrotu[4] = pakiet.substr(a, b - a); // argument a

	a = pakiet.find("+#6:B=") + 6;
	b = pakiet.length();
	doZwrotu[5] = pakiet.substr(a, b - a); // argument b

	return doZwrotu;
}
#pragma endregion operacje na pakietach


int main()
{

	std::map<std::string, int> operacje;
	operacje["ID"] = 0;
	operacje["SUMA"] = 1;
	operacje["ROZNICA"] = 2;
	operacje["MNOZENIE"] = 3;
	operacje["DZIELENIE"] = 4;
	operacje["POTEGOWANIE"] = 5;
	operacje["PIERWIASTKOWANIE"] = 6;
	operacje["LOGARYTM"] = 7;
	operacje["SUMAKWADRATOWLICZB"] = 8;
	operacje["SILNIA"] = 9;
	operacje["ZAKONCZ"] = 11;
	operacje["BLAD"] = 12;
	operacje["ODP"] = 13;

	// Zmienne wykorzystywane do przesy³ania danych
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct sockaddr_in serwer;
	// Zmienne pomocnicze do obs³ugi odbieranych/wysy³anych pakietów
	std::string  pakiet = stworzPakiet("1", "11", "111", "1111", "11111");
	std::string dane = "";
	std::array<std::string, 6> dekodowane = dekodujPakiet(pakiet);
	std::clog << "\n";
	int idSesji = 0;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::clog << "WSAStartup blad: " << iResult << "\n";
		exit(EXIT_FAILURE);
	}
	serwer.sin_family = AF_INET;
	InetPton(AF_INET, IPADDR, &serwer.sin_addr);
	serwer.sin_port = htons(DEFAULT_PORT);

	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		std::clog << "Socket blad: " << WSAGetLastError() << "\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	iResult = connect(ConnectSocket, reinterpret_cast<sockaddr*>(&serwer), sizeof(serwer));
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	if (ConnectSocket == INVALID_SOCKET) {
		std::clog << "Nie mozna sie polaczyc z serwerem\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}


	// Wy³anie pakietu prosz¹cego o przyznanie id
	pakiet = stworzPakiet("ID", "0", "0", "0", "0");
	wyslij(ConnectSocket, pakiet);
	// Odebranie pakietu z id sesji
	dane = odbierz(ConnectSocket);
	dekodowane = dekodujPakiet(dane);
	idSesji = atoi(dekodowane[3].c_str()); // Przypisanie otrzymanego ID do zmiennej
	std::clog << "Id Sesji: " << dekodowane[3] << "\n";
	// Zmienna potrzebna do wyboru akcji podejmowanej przez klienta
	int wybor;

	bool isOk = true;
	do {
		system("cls");
		std::clog << "Wybierz liczbe od 1 do 10\n";
		std::clog << "Menu:\n1.Dodawanie\n2.Odejmowanie\n3.Mnozenie\n" <<
			"4.Dzielenie\n5.Potegowanie\n6.Pierwiastowanie\n7.Logartm\n" <<
			"8.Suma kwadratow liczb\n9.Silnia\n10.Zakoncz\n";
		do {
			isOk = true;
			std::cin >> wybor;
			if (std::cin.fail()) {
				isOk = false;
				std::cin.clear();
				std::cin.ignore(INT_MAX, '\n');
			}
			if (wybor < 0 && wybor>11)
				isOk = false;
			if (!isOk)
				std::clog << "Bledne dane!\n";
		} while (!isOk);

		switch (wybor) {
		case 1: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do dodania:\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("SUMA", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 2: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do odejmowania:\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("ROZNICA", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 3: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do mnozenia:\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("MNOZENIE", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 4: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do dzielenia:\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("DZIELENIE", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 5: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do potegowania (napierw podstawa, pozniej wykladnik):\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("POTEGOWANIE", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 6: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do pierwiastkowania (napierw stopien, pozniej liczba pierwiastkowana):\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("PIERWIASTKOWANIE", "0", intToString(idSesji), floatToString(liczbaB), floatToString(liczbaA));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 7: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby do logarytmowania (napierw podstawa, pozniej liczba logarytmowana):\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail() || liczbaA<0 || liczbaA == 1 || liczbaB<0) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("LOGARYTM", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			
				std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 8: {
			float liczbaA, liczbaB;
			std::clog << "Podaj liczby:\n";
			do {
				isOk = true;
				std::cin >> liczbaA >> liczbaB;
				if (std::cin.fail()) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("SUMAKWADRATOWLICZB", "0", intToString(idSesji), floatToString(liczbaA), floatToString(liczbaB));
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 9: {
			float liczbaA;
			std::clog << "Silnia z liczby:\n";
			do {
				isOk = true;
				std::cin >> liczbaA;
				if (std::cin.fail() || liczbaA<0) {
					isOk = false;
					std::cin.clear();
					std::cin.ignore(INT_MAX, '\n');
				}
				if (!isOk)
					std::clog << "Bledne dane!\n";
			} while (!isOk);
			pakiet = "";
			pakiet = stworzPakiet("SILNIA", "0", intToString(idSesji), floatToString(liczbaA), "0");
			wyslij(ConnectSocket, pakiet);
			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);
			std::clog << "Wynik: " << dekodowane[2] << "\n";
			std::clog << "\nNacisnij dowolny przycisk, aby kontynuowac." << std::endl;
			_getch();
		}
				break;
		case 10: {
			// Wys³anie pakietu koñcz¹cego po³¹czenie 
			pakiet = stworzPakiet("ZAKONCZ", "0", intToString(idSesji), "0", "0");
			std::clog << pakiet << "\n";
			wyslij(ConnectSocket, pakiet);

			dane = odbierz(ConnectSocket);
			dekodowane = dekodujPakiet(dane);

			if (dekodowane[1] == "ZAKONCZ" && dekodowane[2]=="OK") {
				// Zakoñczenie po³¹czenia
				iResult = shutdown(ConnectSocket, SD_SEND);
				if (iResult == SOCKET_ERROR) {
					std::clog << "shutdown blad: " << WSAGetLastError() << "\n";
					closesocket(ConnectSocket);
					WSACleanup();
					exit(EXIT_FAILURE);
				}
				do {
					iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
					if (iResult > 0)
						std::clog << "Bajty odebrane: " << iResult << "\n";
					else if (iResult == 0)
						std::clog << "Polaczenie zamkniete\n";
					else
						std::clog << "recv zakonczone bledem: " << WSAGetLastError() << "\n";
				} while (iResult > 0);
				closesocket(ConnectSocket);
				WSACleanup();

				exit(EXIT_SUCCESS);
			}
		}
				 break;
		}
	} while (wybor != 10);
	exit(EXIT_SUCCESS);
}

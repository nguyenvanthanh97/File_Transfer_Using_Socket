#include <stdio.h>
#include <conio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <iphlpapi.h>
std::string rootPath = "C:";

std::string listDisk;

void SetListDisk(std::string &s){
	char disk[] = " :\\";
	char link[] = "\t\t\t<a class=\"button\" href =\"/?disk= \"> : ";

	char name[100];
	s += "\t\t<div class=\"listdisk\">\n";
	for(char c = 65; c < 91; c++){
		disk[0] = c;
		if(GetVolumeInformation(disk,name,100, NULL, NULL, NULL, NULL, 0)){
			link[35] = c;
			link[38] = c;
			s += link;
			s += name;
			s += "</a>\n";
		}
	}
	s += "\t\t</div>";
}

std::string ExtractField(std::string &s, std::string field){
	int i = s.find(field);
	if(i == std::string::npos) return "";
	std::string value;
	for(int j = i + field.length(); s[j] != '\n' && s[j] != '\r'; j++) value.push_back(s[j]);
	return value;
}

std::string GetPath(std::string &s){
	int i = 0;
	std::string path;
	while(s[++i] != '/');
	path.push_back(s[i]);
	while(s[++i] != ' ') path.push_back(s[i]);
	return path;
}

void FixPath(std::string &s){
	int i = 0, j = 0, l = s.length();
	while(i < l){
		if(s[i] == '%' && s[i+1] == '2' && s[i+2] == '0'){
			s[j] = ' '; i += 3;
		} else {
			s[j] = s[i]; i++;
		}
		j++;
	}
	if(j < s.length()) s.erase(j);
}

std::string CalculateSize(DWORD size){
	char num[10];
	if(size < 1024){
		sprintf(num,"%u bytes",size);
	} else if(size < 1048576){
		sprintf(num,"%.3g KB",size/1024.0);
	} else if(size < 1073741824){
		sprintf(num,"%.3g MB",size/1048576.0);
	} else {
		sprintf(num,"%.3g GB",size/1073741824.0);
	}
	
	return std::string(num);
}

int HandlingPOST(SOCKET &c, std::string &HeaderResponse, std::string &fullPath){
	std::string info;
	int contentLength = std::stoi("0" + ExtractField(HeaderResponse,"Content-Length: "));
	int boundary = ExtractField(HeaderResponse,"boundary=").length();
	if(contentLength > 0 && boundary > 0){
		int check = 0;
		contentLength -= boundary + 6;
		
		//-----Doc thong tin file
		int r;
		char byte;
		while(true){
		    r = recv(c, &byte, 1, 0);
		    if(r < 1) break;
		    contentLength -= r;
			if(byte == '\n') {
				if(check == 1)
					break;
				else
					check = 1;
			} else {
				if(byte != '\r') check = 0;
			}
			info.push_back(byte);
		}
		
		std::string filePath = fullPath + ExtractField(info,"filename=\"");
		filePath.pop_back();
		//-----Doc va ghi noi dung file
		FILE *f;
		f = fopen(filePath.c_str(), "wb");
		if(f == NULL) return 1;
		char *buff = new char[512];
		while(contentLength > 0){
		    r = recv(c, buff, std::min(512,contentLength), 0);
		    if(r < 1) break;
		    contentLength -= r;
			fwrite(buff,r,1,f);
		}

		fclose(f);
		if(r < 1){
			remove(filePath.c_str());
			delete buff;
			return 1;
		}
		r = recv(c, buff, 512, 0);
		delete buff;
		return 0;
	}
}

void ReadHeaders(SOCKET &c, std::string &HeaderResponse){
	int r = 0, check = 0;
	char byte;
	while(true){
	    r = recv(c, &byte, 1, 0);
	    if(r < 1) break;
		if(byte == '\n') {
			if(check == 1)
				break;
			else
				check = 1;
		} else {
			if(byte != '\r') check = 0;
		}
		HeaderResponse.push_back(byte);
	}
}

void SendFileData(SOCKET &c, std::string &fullPath){
	fullPath[fullPath.length()-4] = 0;
	std::string header = 	"HTTP/1.0 200 OK\r\n"
							"Server: NguyenVanThanh File Transfer Server\r\n"
							"Accept-Ranges: bytes\r\n";
	FILE* f = fopen(fullPath.c_str(), "rb");
	if(f == NULL){
		int i = fullPath.find(":File:");
		if(i == 3){
			fullPath = fullPath.substr(i+7);
			f = fopen(fullPath.c_str(), "rb");
		}
	}
	if (f == NULL) {
		f = fopen("errors.html", "rb");
		header += "Content-Type: text/html\r\n"; 
	}

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	header += "Content-Length: ";
	char buf[5];					
	sprintf(buf,"%d",size);
	header += std::string(buf) + "\r\n\r\n";
				
	send(c, header.c_str(), header.length(), 0);
	
	char *fdata = new char[512];
	while(!feof(f)){
		int r = fread(fdata, 1, 512, f);
		send(c, fdata, r, 0);
	}
	
	fclose(f);
	delete fdata;
}

DWORD WINAPI HttpProc(LPVOID arg){
	SOCKET c = (SOCKET)arg;

	std::string HeaderResponse;
	
	//-----Doc Headers
	ReadHeaders(c, HeaderResponse);
	puts(HeaderResponse.c_str());
	//-----Ket thuc headers
	std::size_t i = HeaderResponse.find("HTTP");
	if (i != std::string::npos){
		
		std::string path = GetPath(HeaderResponse);
		
		//---Chon disk
		if(HeaderResponse[0] == 'G' && path.length() == 8 && path.find("?disk=") == 1){
			char disk[] = " :\\";
			disk[0] = path[7];
			if(GetFileAttributes(disk) != INVALID_FILE_ATTRIBUTES) rootPath[0] = path[7];
			path = "/";
		}
		
		//-----Xu ly duong dan
		std::string fullPath = rootPath + path;
		for (int i = 0; i < fullPath.length(); ++i){
			if (fullPath[i] == '/')
				fullPath[i] = '\\';
		}
		FixPath(fullPath);
		
		if(HeaderResponse[0] == 'P'){	//-----Xu ly POST request
			char respond[] = 	"HTTP/1.0 200 OK\r\n"
								"Server: NguyenVanThanh File Transfer Server\r\n"
								"Content-Type: text/html\r\n"
								"Accept-Ranges: bytes\r\n"
								"Content-Length: 1\r\n\r\n1";
			if(HandlingPOST(c, HeaderResponse, fullPath)) respond[130] = '0';
			send(c, respond, 131, 0);
			closesocket(c);
			return 0;
		}
		else if(HeaderResponse[0] == 'D'){	//-----Xu ly DELETE request
				char respond[] = 	"HTTP/1.0 200 OK\r\n"
									"Server: NguyenVanThanh File Transfer Server\r\n"
									"Content-Type: text/html\r\n"
									"Accept-Ranges: bytes\r\n"
									"Content-Length: 1\r\n\r\n1";
				if(remove(fullPath.c_str())) respond[130] = '0';
				send(c, respond, 131, 0);
				closesocket(c);
				return 0;
			}
		
		std::string html =	"<html>\n"
							"\t<head>\n"
							"\t\t<meta charset=\"UTF-8\">\n"
							"\t\t<meta name=\"viewport\" content=\"width=device-width,height=device-height,initial-scale=1.0\"/>\n"
							"\t\t<title>File Transfer by Nguy\u1EC5n V\u0103n Th\u00E0nh</title>\n"
							"\t\t<link rel=\"shortcut icon\" href=\"/:File:/favicon.ico\" />\n"
							"\t\t<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
							"\t\t<link rel=\"stylesheet\" href=\"/:File:/css/style.css\">\n"
							"\t\t<script src=\"https://code.jquery.com/jquery-3.2.1.min.js\"></script>\n"
							"\t\t<script src=\"/:File:/js/style.js\"></script>\n"
							"\t</head>\n"
							"\t<body>\n";
		html += listDisk;
		html +=	"\n\t\t<input id=\"multiFiles\" name=\"files[]\" type=\"file\" multiple=\"multiple\">\n"
				"\t\t<button id=\"upload\" onclick=\"upload('";
		html += path;
		html += "');\">Upload</button>\n";
		html += "\t\t<div id=\"status\">\n\t\t\t<div id=\"filenameupload\"></div>\n\t\t\t<div id=\"processupload\"></div>\n\t\t</div>\n";
			
		fullPath += "\\*.*";
		
		//-----Doc noi dung danh sach thu muc va file
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = FindFirstFile(fullPath.c_str(), &FDATA);
		if (hFind != INVALID_HANDLE_VALUE){
			html += "\t\t<div id=\"listing\">\n";
			do {
				std::string href = path;
				if (href[href.length() - 1] != '/') href += "/";
				href += std::string(FDATA.cFileName);
				if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					html += "\t\t\t<div class=\"1 fa fa-folder\">&nbsp;<a title=\"folder\" href=\"";
					html += href;
					html += "/\"><b>";
					html += std::string(FDATA.cFileName);
					html += "</b></a></div>\n";
				} else {
					html += "\t\t\t<div>&nbsp;<a title=\"file\" href=\"";
					html += href;
					html += "\"><font color=\"#000000\">";
					html += std::string(FDATA.cFileName);
					html += "</font></a> (";
					html += CalculateSize((FDATA.nFileSizeHigh * (MAXDWORD+1)) + FDATA.nFileSizeLow);
					html += ") <button class=\"buttondelete\" onclick=\"remove(this);\"><i class=\"fa fa-trash\"></i> Delete</button></div>\n";
				}
			} while (FindNextFile(hFind, &FDATA) == TRUE);
			
			html += "\t\t</div>\n\t</body>\n</html>";
			std::string header = 	"HTTP/1.0 200 OK\r\n"
									"Server: NguyenVanThanh File Transfer Server\r\n"
									"Content-Type: text/html\r\n"
									"Accept-Ranges: bytes\r\n"
									"Content-Length: ";
			char buf[5];					
			sprintf(buf,"%d",html.length());
			header += std::string(buf) + "\r\n\r\n";
			
			send(c, header.c_str(), header.length(), 0);
			send(c, html.c_str(), html.length(), 0);
			
		} else {
			SendFileData(c, fullPath);
		}

	
	}
	closesocket(c);
	return 0;
}


int CheckName(PIP_ADAPTER_ADDRESSES aa){
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);
	WideCharToMultiByte(CP_ACP, 0, aa->FriendlyName, wcslen(aa->FriendlyName), buf, BUFSIZ, NULL, NULL);
	if(strstr(buf,"Wi-Fi") != NULL) return 1;
	else return 0;
}

std::string GetAddr(PIP_ADAPTER_UNICAST_ADDRESS ua){
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);
	getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
	return std::string(buf);
}

std::string Ipaddress(){
	DWORD rv, size;
	PIP_ADAPTER_ADDRESSES adapter_addresses, aa;
	PIP_ADAPTER_UNICAST_ADDRESS ua;

	rv = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, NULL, &size);
	if (rv != ERROR_BUFFER_OVERFLOW) {
		return "";
	}

	adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(size);

	rv = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, adapter_addresses, &size);
	if (rv != ERROR_SUCCESS) {
		free(adapter_addresses);
		return "";
	}

	for (aa = adapter_addresses; aa != NULL; aa = aa->Next) {
		if(CheckName(aa)) return GetAddr(aa->FirstUnicastAddress);
	}

	free(adapter_addresses);
}

void gotoxy(short x, short y){
    static HANDLE  h = NULL;
    if(!h)
        h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD c = {x,y};
    SetConsoleCursorPosition(h,c);
}

int Menu(){
	int chon = 0, choncu = 0, h = 6;
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 10;
	info.bVisible = false;
	SetConsoleCursorInfo(consoleHandle, &info);
	puts("Do you want to change ip address and port? ");
	gotoxy(32,h);
	printf("No");
	gotoxy(32,h+1);
	printf("Yes");
	gotoxy(25,h); printf("-->");
	while(1){
		char phim;
		if(kbhit()){
			phim = getch();
			if(phim == 13) break;
 			if(phim == 80){
 				gotoxy(25,h + chon); printf("   ");
				if(chon < 1)
					chon++;
				else
					chon = 0;
				gotoxy(25,h + chon); printf("-->");
			}
			if(phim == 72){
				choncu = chon;
				gotoxy(25,h + chon); printf("   ");
				if(chon > 0)
					chon--; 
				else 
					chon = 1;	
				gotoxy(25,h + chon); printf("-->");
			}
		}
	}
	info.bVisible = true;
	SetConsoleCursorInfo(consoleHandle, &info);
	return chon;
}


int main(){
	system("Color F1");
	SetConsoleTitle("File Transfer by Nguyen Van Thanh");
	WSADATA DATA;
	puts("------------------------------------------------------");
	puts("File Transfer Software developed by Nguyen Van Thanh");
	puts("Contact me at: NguyenVanThanh97HUST@gmail.com");
	puts("Facebook: https://www.facebook.com/thanhnv03");
	puts("------------------------------------------------------");
	if(WSAStartup(MAKEWORD(2, 2), &DATA)){
		puts("Error!");
		return 0;
	}
	
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET){
		puts("Error!");
		return 0;
	}
	
	std::string ip = Ipaddress();
	unsigned int port = 3340;
	if(ip.length() != 0)
		printf("Open others device have the same network internet and connect to the address http://%s:%d\n",ip.c_str(), port);
	if(ip.length() == 0 || Menu()){
		system("cls");
		puts("------------------------------------------------------");
		puts("File Transfer Software developed by Nguyen Van Thanh");
		puts("Contact me at: SongTinhCam03@gmail.com");
		puts("------------------------------------------------------");
		char newip[20];
		printf("Input ip address: "); scanf("%s",newip);
		while(inet_addr(newip) == INADDR_NONE){
			printf("Error ip address, input again: "); scanf("%s",newip);
		}
		ip = std::string(newip);
		
		printf("Input port (0-65535): "); scanf("%d",&port);
		while(-1 < port && port < 65536){
			printf("Error port, input again: "); scanf("%d",&port);
		}
		printf("Open others device have the same network internet and connect to the address http://%s:%d\n",ip.c_str(), port);
	} else {
		system("cls");
		puts("------------------------------------------------------");
		puts("File Transfer Software developed by Nguyen Van Thanh");
		puts("Contact me at: SongTinhCam03@gmail.com");
		puts("------------------------------------------------------");
		printf("Open another device have the same network internet and connect to the address http://%s:%d\n",ip.c_str(), port);
	}
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	bind(s, (sockaddr*)&saddr, sizeof(saddr));
	listen(s, 10);
	puts("The program is running!");
	ip.clear();
	SetListDisk(listDisk);
	while (true){
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (sockaddr*)&caddr, &clen);
		CreateThread(NULL, 0, HttpProc, (LPVOID)c, 0, NULL);
	}
}

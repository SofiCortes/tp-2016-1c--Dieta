#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "funciones.h"
#include "commons/log.h"
#include <sockets.h>
#include <protocolo.h>
#include <structs.h>
#include <parser/metadata_program.h>
#include "primitivas.h";
#include "variables_globales.h"

AnSISOP_funciones functions = {
		.AnSISOP_definirVariable = definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar = dereferenciar,
		.AnSISOP_asignar = asignar,
		.AnSISOP_imprimir = imprimir,
		.AnSISOP_imprimirTexto = imprimirTexto,

};

AnSISOP_kernel kernel_functions = { };

int main(int argc, char *argv[]) {
	//Recibe el archivo de config por parametro
	/*if (argc != 2) {
		printf("Número incorrecto de parámetros\n");
		return -1;
	}

	t_config* config = config_create(argv[1]);*/

	//t_config* config;
	if (argc != 2) {
		config = config_create("./Configuracion/config");
	} else {

		config = config_create(argv[1]);
	}


	t_log* logger;
	logger = log_create("Consola.log", "CONSOLA", 1,
			log_level_from_string("INFO"));
	char *texto;
	texto = "info";

	int puerto_umc = config_get_int_value(config,"PUERTO_UMC");
	int puerto_nucleo = config_get_int_value(config,"PUERTO_NUCLEO");
	char* ip_umc = config_get_string_value(config,"IP_UMC");
	char* ip_nucleo = config_get_string_value(config,"IP_NUCLEO");

	//creo socket nucleo
	int socketNucleo;
	crearSocket(&socketNucleo);

	//me intento conectar
	if (conectarA(socketNucleo, ip_nucleo, puerto_nucleo)){
		perror("No se pudo conectar");
		log_error(logger, "No se pudo conectar a la UMC", texto);
		return 1;
	}
	//handshake
	if (responderHandshake(socketNucleo, IDCPU, IDNUCLEO)){
		log_error(logger, "Error en el handshake", texto);
		return 1;
	}

	//creo socket umc
	int socketUMC;
	crearSocket(&socketUMC);

	//me intento conectar
	if (conectarA(socketUMC, ip_umc, puerto_umc)){
		perror("No se pudo conectar");
		log_error(logger, "No se pudo conectar a la UMC", texto);
		return 1;
	}
	//handshake
	if (responderHandshake(socketNucleo, IDCPU, IDUMC)){
		log_error(logger, "Error en el handshake", texto);
		return 1;
	}

	if (recibirHeader(socketNucleo)==headerPcb){
		t_pcb pcbRecibido ;
		pcbRecibido = recibirPcb(socketNucleo);
		int tamanioPagina = recibirTamanioPagina(socketUMC);
		t_posicion_memoria posicionPagina = obtenerPosicionPagina(tamanioPagina, pcbRecibido);
		char lineaAnsisop [posicionPagina.size];
		recibirLineaAnsisop(socketUMC, posicionPagina, &lineaAnsisop);
		analizadorLinea(strdup(lineaAnsisop), &functions, &kernel_functions);
	}


	//socket cliente nucleo
	struct sockaddr_in direccionServidorNucleo;
	direccionServidorNucleo.sin_family = AF_INET;
	direccionServidorNucleo.sin_addr.s_addr = inet_addr(ip_nucleo);
	direccionServidorNucleo.sin_port = htons(puerto_nucleo);

	int clienteNucleo = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(clienteNucleo, (void*) &direccionServidorNucleo, sizeof(direccionServidorNucleo)) != 0) {
		perror("No se pudo conectar al nucleo");
		return 1;
	}

	//recibo ruta del nucleo
	char* ruta = malloc(30);
	int bytesRecibidosRuta = recv(clienteNucleo, ruta, 30, 0);
	ruta[bytesRecibidosRuta] = '\0';

	//recibo mensaje del nucleo
	char* buffer = malloc(10);
	int bytesRecibidos = recv(clienteNucleo, buffer, 10, 0);
	buffer[bytesRecibidos] = '\0';
	printf("Nucleo dice: %s\n", buffer);

	//cliente de UMC
	struct sockaddr_in direccionServidorUMC;
	direccionServidorUMC.sin_family = AF_INET;
	direccionServidorUMC.sin_addr.s_addr = inet_addr(ip_umc);
	direccionServidorUMC.sin_port = htons(puerto_umc);

	int clienteUMC = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(clienteUMC, (void*) &direccionServidorUMC, sizeof(direccionServidorUMC)) != 0) {
		perror("No se pudo conectar a la UMC");
		return 1;
	}


	send(clienteUMC, buffer, strlen(buffer), 0);


	//parser
	/*FILE* archivo;
	archivo = fopen(ruta, "r");
	if (archivo == NULL) {
		puts("ERROR");
	}
	free(ruta);

	int largoLinea = 30;
	char *linea = (char *) malloc(sizeof(char) * largoLinea); //Buffer de linea

	char caracter = getc(archivo);
	int codigoValido = 0;	//Flag. Indica si esta dentro del begin-end

	while (caracter != EOF) {	//Lee linea por linea
		int contador = 0;

		while ((caracter != '\n') && (caracter != EOF)) {
			linea[contador] = caracter;
			contador++;
			caracter = getc(archivo);
		}

		linea[contador] = '\0';

		int comentario = 0;
		int i;
		for (i = 0; i < contador; i++) {
			if (linea[i] == '#') {
				comentario = 1;
			}
		}

		if (!comentario) { //Saltea lineas de comentario

			if (!strcmp(linea, "end")) {
				codigoValido = 0;
			}

			//Ejecutar parser con la Linea
			if (codigoValido) {
				printf("%s\n", linea);

				analizadorLinea(strdup(linea), &functions, &kernel_functions);
			}

			if (!strcmp(linea, "begin")) {
				codigoValido = 1;

			}
		}

		caracter = getc(archivo);
	}*/

	return 0;

}
//primitivas
//puedo pasar esto a otro archivo



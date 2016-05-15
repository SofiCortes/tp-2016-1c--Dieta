/*
 * protocolo.h
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <arpa/inet.h>
#include <sys/socket.h>

enum headers { //Constantes que identifican los headers de los mensajes

	programaAnsisop = 1
};
int recibirHeader(int socketOrigen);
void enviarProgramaAnsisop(int socketDestino, char * codigo, int largoCodigo);
void recibirProgramaAnsisop(int socketOrigen, char * codigo);

#endif /* PROTOCOLO_H_ */
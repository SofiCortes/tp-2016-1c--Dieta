/*
 * primitivas.h
 *
 *  Created on: 20/5/2016
 *      Author: utnso
 */

#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <commons/log.h>
#include <string.h>
#include <commons/string.h>
#include <stdint.h>
#include <commons/config.h>
#include "variables_globales.h"
#include <protocolo.h>
#include <pila.h>

t_puntero definirVariable(t_nombre_variable variable);
t_puntero obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable dereferenciar(t_puntero puntero);
void asignar(t_puntero puntero, t_valor_variable variable);
int imprimir(t_valor_variable valor);
int imprimirTexto(char* texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
void irAlLabel(t_nombre_etiqueta etiqueta);
void retornar(t_valor_variable retorno);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);


#endif /* PRIMITIVAS_H_ */

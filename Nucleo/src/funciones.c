/*
 * funciones.c
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#include "funciones.h"

void manejarCPU(void * socket) {

	/*
	 * chequeo que el CPU no haya hecho un hot plug
	 * t_pcb unPcb = obtenerPcbEnEstadoDeListo();
	 * Se ejecuta un quantum
	 * Se pone el pcb en estado de ejecucion
	 * Se envia pcb al cpu
	 * termina cuantum/interrupcion
	 * Se pone pcb en cola de listo/bloqueado segun corresponda
	 * repetir
	 */

	int socketCpu;
	memcpy(&socketCpu, socket, sizeof(int));
	free(socket);

	enviarUnidadesQuantum(socketCpu, cantidadQuantum);
	enviarSleepQuantum(socketCpu, retardoQuantum);

	int desconectado = 0, cambioProceso;

	while (!desconectado) {

		cambioProceso = 0;

		sem_wait(&semaforoColaListos);
		t_pcbConConsola siguientePcb = DevolverProcesoColaListos();
		if (siguientePcb.socketConsola != -1) {
			enviarPcb(socketCpu, siguientePcb.pcb);

			int respuesta;

			while (!cambioProceso) {
				if (recibirRespuestaCPU(socketCpu, &respuesta)) {
					//Se desconecto el CPU
					finalizarProceso(siguientePcb);
					desconectado = 1;
					log_info(logger, "Se desconectó el CPU socket %d",
							socketCpu);
					pthread_exit(NULL);
				}

				switch (respuesta) {

				case finalizacionPrograma: //Fin programa
					if (recibirHeader(socketCpu) == headerPcb) {
						siguientePcb.pcb = recibirPcb(socketCpu);
						finalizarProceso(siguientePcb);
						cambioProceso = 1;
					} else {
						log_error(logger, "El CPU no envio un Pcb", texto);
						finalizarProceso(siguientePcb);
						pthread_exit(NULL);
					}
					break;
				case abortarPrograma:
					//Fin programa por abortado
					if (recibirHeader(socketCpu) == headerPcb) {
						siguientePcb.pcb = recibirPcb(socketCpu);
						abortarProceso(siguientePcb);
						cambioProceso = 1;
					} else {
						log_error(logger, "El CPU no envio un Pcb", texto);
						finalizarProceso(siguientePcb);
						pthread_exit(NULL);
					}
					break;

				case finDeQuantum:
					//Fin quantum

					if (recibirHeader(socketCpu) == headerPcb) {
						siguientePcb.pcb = recibirPcb(socketCpu);

						int j, sizeLista = list_size(
								listaFinalizacionesPendientes);
						int * socketEnLista;

						pthread_mutex_lock(&mutexListaFinalizacionesPendientes);

						for (j = 0; j < sizeLista; j++) {
							socketEnLista = (int *) list_get(
									listaFinalizacionesPendientes, j);
							if (siguientePcb.socketConsola == *socketEnLista) {
								finalizarProceso(siguientePcb);
								list_remove(listaFinalizacionesPendientes, j);
								AgregarAProcesoColaFinalizados(siguientePcb);
								cambioProceso = 1;
								j = sizeLista;
								free(socketEnLista);
							}
						}
						pthread_mutex_unlock(
								&mutexListaFinalizacionesPendientes);

						if (!cambioProceso) {
							AgregarAProcesoColaListos(siguientePcb);
							cambioProceso = 1;
						}
					} else {
						log_error(logger, "El CPU no envio un Pcb", texto);
						finalizarProceso(siguientePcb);
						pthread_exit(NULL);
					}
					log_info(logger, "Quantum de CPU socket %d terminado",
							socketCpu);
					break;

				case primitivaImprimir:
					;
					int largoTexto;
					char *texto;
					uint32_t pid;
					recibirValorAImprimir(socketCpu, &pid, &largoTexto, &texto);
					int listSize = list_size(listaConsolas), i;

					pthread_mutex_lock(&mutexListaConsolas);
					for (i = 0; i < listSize; i++) {
						t_pcbConConsola * elemento =
								(t_pcbConConsola *) list_get(listaConsolas, i);
						if (elemento->pcb.pid == pid) {
							enviarResultadoDeEjecucionAnsisop(
									elemento->socketConsola, texto, largoTexto);
							log_info(logger,
									"Se envió texto a imprimir, del proceso PID: %d a Consola (socket nro %d)",
									elemento->pcb.pid, elemento->socketConsola);
							break;
						}
					}
					pthread_mutex_unlock(&mutexListaConsolas);

					break;

				case headerEntradaSalida:
					if (recibirHeader(socketCpu) == headerPcb) {
						siguientePcb.pcb = recibirPcb(socketCpu);

						int largo, tiempo;
						char * nombre;
						recibirEntradaSalida(socketCpu, &largo, &nombre,
								&tiempo);
						ponerEnColaBloqueados(siguientePcb, nombre, largo,
								tiempo);
						cambioProceso = 1;
					}

					break;

				case finalizacionCPU:
					desconectado = 1;
					cambioProceso = 1;
					log_info(logger, "CPU socket %d apagado", socketCpu);

				}
			}
		}
	}
	pthread_exit(NULL);
}

void AgregarACola(t_pcbConConsola elemento, t_queue * cola) {
	void * nuevoElemento = malloc(sizeof(t_pcbConConsola));
	memcpy(nuevoElemento, &elemento, sizeof(t_pcbConConsola));
	queue_push(cola, nuevoElemento);
}

t_pcbConConsola sacarPrimeroCola(t_queue * cola) {
	t_pcbConConsola elemento;
	void * elementoPop = queue_pop(cola);
	if (elementoPop == NULL) {
		elemento.socketConsola = -1;
		return elemento;
	}
	memcpy(&elemento, elementoPop, sizeof(t_pcbConConsola));
	free(elementoPop);
	return elemento;
}

t_pcbBloqueado sacarPrimeroColaBloqueados(t_queue * cola) {
	t_pcbBloqueado elemento;
	void * elementoPop = queue_pop(cola);
	if (elementoPop == NULL) {
		elemento.pcb.socketConsola = -1;
		return elemento;
	}
	memcpy(&elemento, elementoPop, sizeof(t_pcbBloqueado));
	free(elementoPop);
	return elemento;
}

t_pcbConConsola DevolverProcesoColaListos() {
	pthread_mutex_lock(&mutexColaListos);
	t_pcbConConsola pcb = (sacarPrimeroCola(cola_PCBListos));
	pthread_mutex_unlock(&mutexColaListos);
	return pcb;
}

t_pcbConConsola DevolverProcesoColaFinalizados() {
	pthread_mutex_lock(&mutexColaFinalizados);
	t_pcbConConsola pcb = (sacarPrimeroCola(cola_PCBFinalizados));
	pthread_mutex_unlock(&mutexColaFinalizados);
	return pcb;
}

void AgregarAProcesoColaListos(t_pcbConConsola elemento) {
	pthread_mutex_lock(&mutexColaListos);
	AgregarACola(elemento, cola_PCBListos);
	pthread_mutex_unlock(&mutexColaListos);
	sem_post(&semaforoColaListos);
}

void AgregarAProcesoColaNuevos(t_pcbConConsola elemento) {
	AgregarACola(elemento, cola_PCBNuevos);
}

void AgregarAProcesoColaFinalizados(t_pcbConConsola elemento) {
	pthread_mutex_lock(&mutexColaFinalizados);
	AgregarACola(elemento, cola_PCBFinalizados);
	pthread_mutex_unlock(&mutexColaFinalizados);
}

void AgregarAProcesoColaBloqueados(t_queue * cola, t_pcbBloqueado elemento) {
	void * nuevoElemento = malloc(sizeof(t_pcbBloqueado));
	memcpy(&nuevoElemento, &elemento, sizeof(t_pcbConConsola));
	queue_push(cola, &nuevoElemento);
	return;
}

t_pcb crearPcb(char * programa, int largoPrograma) {
	t_pcb nuevoPcb;
	t_metadata_program * metadata;
	nuevoPcb.pid = pidPcb;
	pidPcb++;

	metadata = metadata_desde_literal(programa);

	nuevoPcb.pc = metadata->instruccion_inicio;
	nuevoPcb.indice_etiquetas.etiquetas = metadata->etiquetas;
	nuevoPcb.indice_etiquetas.largoTotalEtiquetas = metadata->etiquetas_size;

	nuevoPcb.indice_codigo.instrucciones = metadata->instrucciones_serializado;
	nuevoPcb.indice_codigo.cantidadInstrucciones = metadata->instrucciones_size;
	nuevoPcb.indice_codigo.numeroInstruccionInicio =
			metadata->instruccion_inicio;

	nuevoPcb.paginas_codigo = calcularPaginasCodigo(largoPrograma);

	t_list * pilaInicial;
	pilaInicial = list_create();

	t_registro_pila * registroPila = malloc(sizeof(t_registro_pila));
	registroPila->lista_argumentos = list_create();
	registroPila->lista_variables = list_create();
	registroPila->posicionUltimaVariable = nuevoPcb.paginas_codigo
			* tamanioPagina;

	list_add(pilaInicial, (void *) registroPila);

	nuevoPcb.indice_stack = pilaInicial;

	free(metadata);

	return nuevoPcb;
}

int calcularPaginasCodigo(int largoPrograma) {
	int paginas = 0;
	paginas = largoPrograma / tamanioPagina;
	if (largoPrograma % tamanioPagina) {
		paginas++;
	}
	return paginas;

}

int iniciarUnPrograma(int clienteUMC, t_pcb nuevoPcb, int largoPrograma,
		char * programa, uint32_t paginasStack) {
	enviarInicializacionPrograma(clienteUMC, nuevoPcb.pid, largoPrograma,
			programa, nuevoPcb.paginas_codigo + paginasStack);
	return recibirRespuestaInicializacion(clienteUMC);

}

void finalizarProceso(t_pcbConConsola siguientePcb) {
	enviarFinalizacionProgramaUMC(clienteUMC, siguientePcb.pcb.pid);
	enviarFinalizacionProgramaConsola(siguientePcb.socketConsola);

	pthread_mutex_lock(&mutexListaConsolas);
	int largoLista = list_size(listaConsolas), i;
	for (i = 0; i < largoLista; i++) {
		t_pcbConConsola * pcbBusqueda = (t_pcbConConsola *) list_get(
				listaConsolas, i);
		if (pcbBusqueda->socketConsola == siguientePcb.socketConsola) {
			t_pcbConConsola * pcbFinalizado = (t_pcbConConsola *) list_remove(
					listaConsolas, i);
			AgregarAProcesoColaFinalizados(*pcbFinalizado);
			free(pcbFinalizado); //Liberar Pcb
			break;
		}
	}
	pthread_mutex_unlock(&mutexListaConsolas);
	log_info(logger, "Se finalizó programa pid %d", siguientePcb.pcb.pid);

}

void abortarProceso(t_pcbConConsola siguientePcb) {

	enviarFinalizacionProgramaConsola(siguientePcb.socketConsola);

	pthread_mutex_lock(&mutexListaConsolas);
	int largoLista = list_size(listaConsolas), i;
	for (i = 0; i < largoLista; i++) {
		t_pcbConConsola * pcbBusqueda = (t_pcbConConsola *) list_get(
				listaConsolas, i);
		if (pcbBusqueda->socketConsola == siguientePcb.socketConsola) {
			t_pcbConConsola * pcbFinalizado = (t_pcbConConsola *) list_remove(
					listaConsolas, i);
			AgregarAProcesoColaFinalizados(*pcbFinalizado);
			free(pcbFinalizado);
		}
	}
	pthread_mutex_unlock(&mutexListaConsolas);
	log_info(logger, "Se abortó programa pid %d", siguientePcb.pcb.pid);

}

void manejarIO(t_parametroThreadDispositivoIO * datosHilo) {

	while (1) {

		t_pcbBloqueado pedidoDeIO;
		sem_wait(datosHilo->semaforo);
		pthread_mutex_lock(datosHilo->mutex);
		pedidoDeIO = sacarPrimeroColaBloqueados(datosHilo->colaBloqueados);
		pthread_mutex_unlock(datosHilo->mutex);

		usleep(
				pedidoDeIO.unidadesTiempoIO * datosHilo->retardoDispositivo
						* 1000);
		log_info(logger, "Programa pid %d termino IO", pedidoDeIO.pcb.pcb.pid);

		int sizeLista = list_size(listaFinalizacionesPendientes), encontrado =
				-1, i;

		for (i = 0; i < sizeLista; i++) {

			int * socket = list_get(listaFinalizacionesPendientes, i);
			if (*socket == pedidoDeIO.pcb.socketConsola) {
				encontrado = pedidoDeIO.pcb.socketConsola;
				free(list_remove(listaFinalizacionesPendientes, i));
			}
		}
		if (encontrado != -1) {
			AgregarAProcesoColaListos(pedidoDeIO.pcb);
		} else {
			finalizarProceso(pedidoDeIO.pcb);
		}

	}

}

void crearHilosEntradaSalida() {

	int contador = 0, i;

	while (vectorDispositivos[contador] != NULL) {
		contador++;
	}
	vectorColasBloqueados = malloc(sizeof(t_queue *) * contador);
	vectorMutexDispositivosIO = malloc(sizeof(pthread_mutex_t *) * contador);
	vectorSemaforosDispositivosIO = malloc(sizeof(sem_t) * contador);

	for (i = 0; i < contador; i++) {
		t_parametroThreadDispositivoIO * parametro = malloc(
				sizeof(t_parametroThreadDispositivoIO));
		vectorColasBloqueados[i] = (t_queue *) list_create();
		parametro->colaBloqueados = vectorColasBloqueados[i];
		vectorMutexDispositivosIO[i] = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(vectorMutexDispositivosIO[i], NULL);
		parametro->mutex = vectorMutexDispositivosIO[i];
		parametro->retardoDispositivo = atoi(vectorRetardoDispositivos[i]);
		sem_init(&(vectorSemaforosDispositivosIO[i]), 1, 0);
		parametro->semaforo = &(vectorSemaforosDispositivosIO[i]);

		pthread_t nuevoHilo;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&nuevoHilo, &attr, (void *) manejarIO, parametro);
		pthread_attr_destroy(&attr);

	}
}

void ponerEnColaBloqueados(t_pcbConConsola siguientePcb, char * nombre,
		int largo, int tiempo) {

	int j, sizeLista = list_size(listaFinalizacionesPendientes), encontrado = 0;
	int * socketEnLista;

	pthread_mutex_lock(&mutexListaFinalizacionesPendientes);

	for (j = 0; j < sizeLista; j++) {
		socketEnLista = (int *) list_get(listaFinalizacionesPendientes, j);
		if (siguientePcb.socketConsola == *socketEnLista) {
			finalizarProceso(siguientePcb);
			list_remove(listaFinalizacionesPendientes, j);

			pthread_mutex_unlock(&mutexListaFinalizacionesPendientes);
			encontrado = 1;
			AgregarAProcesoColaFinalizados(siguientePcb);
			j = sizeLista;
		}
	}

	if (!encontrado) {
		t_pcbBloqueado pcbBloqueado;
		pcbBloqueado.pcb = siguientePcb;
		pcbBloqueado.unidadesTiempoIO = tiempo;

		int contador = 0, i, existeDispositivo = 0;

		while (vectorDispositivos[contador] != NULL) {
			contador++;
		}

		for (i = 0; i < contador; i++) {

			if (!strcmp(vectorDispositivos[i], nombre)) {
				pthread_mutex_lock(vectorMutexDispositivosIO[i]);
				AgregarAProcesoColaBloqueados(vectorColasBloqueados[i],
						pcbBloqueado);
				pthread_mutex_unlock(vectorMutexDispositivosIO[i]);
				sem_post(&(vectorSemaforosDispositivosIO[i]));
				existeDispositivo = 1;
			}

		}
		if (!existeDispositivo) {
			log_error(logger,
					"No existe el dispositivo solicitado por el proceso pid %d",
					pcbBloqueado.pcb.pcb.pid);
		}

	}

}

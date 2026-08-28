#ifndef VIMMON_PL_MUNDO_H
#define VIMMON_PL_MUNDO_H

// ============================================================
// VimMon — LIBRERIA `mundo` — VERBOS 3D PARA PAED
//
// Esto NO es parte del lenguaje PAED, igual que plugins/ide/escena.c no lo
// era. Es un HOST: un programa que hospeda al interprete y le agrega verbos
// propios. El interprete no sabe que existe un juego; el juego se ANOTA.
//
// Une dos contratos que ya existian y no se conocian entre si:
//
//   paed/lang/include/paed/interpreter.h   <- el lenguaje
//   plugins/renderer3d/renderer3d.h        <- la GPU
//
// Once verbos, dos familias:
//
//   FUNCIONES (devuelven un valor, se usan dentro de una expresion)
//     SALIR()      1 si cerraron la ventana
//     TECLA(t)     1 si la tecla esta apretada
//     MOUSE_X()    cuanto se movio el mouse en horizontal desde el frame pasado
//     MOUSE_Y()    idem en vertical
//     CLIC(b)      1 si el boton del mouse esta apretado
//     TICKS()      milisegundos desde que arranco
//
//   PROCEDIMIENTOS (hacen algo, no devuelven nada)
//     INICIAR(titulo =, ancho =, alto =)
//     CAPTURAR_MOUSE(activar =)
//     FRAME_INICIO(camx =, camy =, camz =, angulo =, cielo =)
//     CUBO(x =, y =, z =, color =)
//     BILLBOARD(x =, y =, z =, ancho =, alto =, color =)
//     FRAME_FIN()
//
// Las FUNCIONES no se declaran en ningun .json: expr.c las resuelve en tiempo
// de ejecucion. Los PROCEDIMIENTOS si: el parser los valida contra
// paed/data/mundo.json ANTES de ejecutar nada.
// ============================================================

// Anota los once verbos en el interprete. Llamar UNA vez, antes de interp_exec.
// Devuelve 0 si quedaron todos anotados.
int mundo_registrar(void);

// Apaga el motor si INICIAR llego a abrirlo. La llama el host DESPUES de
// ejecutar el programa: un .paed no tiene por que acordarse de cerrar la
// ventana, igual que no cierra la consola cuando termina.
//
// Sin esto el proceso muere con segfault: los recursos de GPU quedan vivos
// cuando el sistema ya desarmo lo suyo, y el orden de destruccion importa.
void mundo_apagar(void);

#endif // VIMMON_PL_MUNDO_H

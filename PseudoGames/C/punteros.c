//Punteros
// scope: Significa que es el area de
// visibilidad de una variable donde existe y
// donde podes usarla. Es como el limite interno de los punteros
//
// Imagina que tengo un archivo de registros de DNI y tengo que pasarle a otro archivo los registros de
// 40 millones y a otro los de 20 millones y asi...

//Nodo registro
struct nodoRegistro{
	int dato = 1;
	struct Nodo *siguienteNodo; // puntero al proximo nodo
};
if

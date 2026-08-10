INSERT INTO niveles (numero, titulo, enunciado, pista, casos_prueba) VALUES
(1, 'Precio con inflación', E'Escribir un programa que permita calcular el precio de un artículo para un año dado, considerando que la inflación es del 4 por 100 anual.\n\nFórmula: P = C * (1 + R) ^ (N - A)\n\nDonde:\n  C = Precio actual\n  R = Tasa de inflación (0.04)\n  N = Año futuro\n  A = Año actual',
'Recordá que R = 0.04 (4/100). Elevás (1+R) a la potencia (N-A) y multiplicás por C.',
'[{"entrada": {"C": 100, "R": 0.04, "A": 2024, "N": 2027}, "salida": 112.4864}]'),

(2, 'Discriminante de ecuación cuadrática', E'Las raíces de una ecuación de segundo grado ax² + bx + c = 0 son reales si y solo si el discriminante dado por (b² - 4ac) no es negativo.\n\nSe desea leer los coeficientes a, b, c e imprimir el resultado del discriminante.\n\nFórmula: D = b² - 4 * a * c',
'Elevás b al cuadrado, luego restás 4 * a * c. Ej: si b=5, b² = 25.',
'[{"entrada": {"a": 1, "b": 5, "c": 2}, "salida": 17}, {"entrada": {"a": 2, "b": 4, "c": 2}, "salida": 0}]'),

(3, 'Precio total PC + Impresora', E'Se desea comprar una PC y una impresora. Calcular el precio total: el cual está dado por la suma de los precios de costos, los porcentajes de ganancia del vendedor y un 21% de IVA.\n\nGanancia del vendedor:\n  - 12% por la PC\n  -  7% por la impresora\n\nFórmula:\n  precioPC = costoPC * 1.12\n  precioImp = costoImpresora * 1.07\n  total = (precioPC + precioImp) * 1.21',
'Primero sumá la ganancia a cada producto, luego sumá ambos y aplicá el 21% de IVA.',
'[{"entrada": {"costoPC": 1000, "costoImpresora": 500}, "salida": 2002.55}]');

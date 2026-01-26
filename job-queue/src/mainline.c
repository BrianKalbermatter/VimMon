int main() // Esto es un hilo = 1
{
    pthread_t hilo;
    int resultado;

    // Esta funcion crea el hilo
    resultado = pthread_create(&hilo,NULL, saludo_hilo, NULL);

    if (resultado != 0)
    {
       
}

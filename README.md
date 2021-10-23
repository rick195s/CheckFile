# CheckFile

A C program that will check if a file really matches his own extension

Preferi passar o summary por parametro para registar os status dos ficheiros em vez de alterar os valores de summary com o valor de retorno de fileProcessing nas funcoes de dirProcessing e batchProcessing pois se o fizesse iria estar a adicionar mais comparações desnecessárias ao programa para verificar se o retorno da funcao era porque o ficheiro era mismatch, ok ou erro.

Utilizei a alocacao de memoria dinamica pois queria tornar o programa um pouco mais eficiente a nível de gestão de memória.

In fileProcessing im using "access()" function because I dont want to open the file, I just want to verify if the bash program "file" will have access to read from it 

I used global variables to show the information when process receive SIGUSR1 signal because it was the only correct way, that I know, to do it

At fileProcessing when file its checked for errors and permissions I dont use exit() or ERROR macro because if I did that the other files in the list wouldn't be processed  

I didn't used debug.h and debug.c because I wanted custom messages and I don't know if I can modify the code in it because it wasn't written by me
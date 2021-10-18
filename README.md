# CheckFile

A C program that will check if a file really matches his own extension

Preferi passar o summary por parametro para registar os status dos ficheiros em vez de alterar os valores de summary com o valor de retorno de fileProcessing nas funcoes de dirProcessing e batchProcessing pois se o fizesse iria estar a adicionar mais comparações desnecessárias ao programa para verificar se o retorno da funcao era porque o ficheiro era mismatch, ok ou erro.

Utilizei a alocacao de memoria dinamica pois queria tornar o programa um pouco mais eficiente.

In fileProcessing im using "access()" function because I dont want to open the file, I just want to verify if the bash program "file" will have access to read from it 


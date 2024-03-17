tamidsh.c 

SHELL PART 1
project explanation:
this program takes user input.
upon getting the user input, the program forks the process and the child program will execute.
the parent waits for the child process to finish before the program prompts the user again.

code explanation:
i take the user input and i create an empty array that will contain all the user's arguments.
the array[0] will be the path of the arguments. 
the following indexes will be each argument stripped individually using strtok.
after adding all arguments to the array, i add a NULL following the last argument to help iterate through them.
i then print the array's contents

bug explanation:
it posts the arguments but im not sure why it prints something in front of the ">" in the beginning.
i wasn't sure if it should print the contents in array form or if it can just be fluid.
(array form: ["hello", "world"] or fluid: hello world)
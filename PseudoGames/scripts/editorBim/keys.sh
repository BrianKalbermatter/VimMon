#!/bin/bash
# Aca van a ir todas las teclas que se van a usar en los modos!
readKey() {
    KEY=""
    read -rsn1 char
    if [[ "$char" == $'\e' ]]; 
    then
        read -rsn2 -t 0.05 rest
        if [[ "$rest" == "[A" ]];
        then


}

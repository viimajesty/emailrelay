#!/bin/bash

source ./variables.sh

echo "the set user is $FROM"
echo "the set password is $PASSWORD"
echo "the set recipient is $SENDTO"
echo "the message to send (argument) is $1"



# Check if all are defined
if [ -z "${FROM+x}" ]; then
    echo "ERR: the variable FROM is unset. This should be set to your Gmail address."
elif [ -z "${PASSWORD+x}" ]; then
    echo "ERR: the variable PASSWORD is unset. This should be set to your Gmail password."
elif [ -z "${SENDTO+x}" ]; then
    echo "ERR: the variable SENDTO is unset. This should be set to the receiver's address."
elif [ -z "${1+x}" ]; then
    echo "You need to specify a message to send. Example: ./launch.sh 'hello world!'"
else
    echo "All variables are set."

    curl -X POST http://localhost:5555/sendmail -d "email=$SENDTO" -d "message=$1" -d "subject=email relay" -d "password=$PASSWORD" -d "from=$FROM"
fi


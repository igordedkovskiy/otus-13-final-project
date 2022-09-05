# Otus Final Project: Producer - Consumer

## Thread safe queue

* thread-safe
* keep any type
* serializable
* tests

## One producer, one consumer (sql server)

* producer - server, that puts queries into queue
* consumer - gets queries from queue and handles them
* producer and consumer work in a different threads
* save queue on quit and on timeout
* load queue on app start

## Multiple producer - consumer (sql server)

* producers - servers, that put queries into queue
* consumers - get queries from queue and handle them
* each producer works in a separate thread
* each consumer works in a separate thread
* save queue on quit and on timeout
* load queue on app start
* each consumer handles queries of N clients at most.

** Client - query transmitter. Producer is not a client. Producer can recieve queries from multiple clients.

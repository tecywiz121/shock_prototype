Shock HTTP Server
=================

Shock is a web server written for an Arduino. This is one of my first Arduino
projects, so the code is a bit of a mess.

The goal is to support HTTP/1.1, especially chunked encoding. Everything is
designed so that only small parts of the request/response have to be in memory
at a time.

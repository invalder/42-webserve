#!/usr/bin/env python
# hello.py

# Enable debugging
import cgitb
cgitb.enable()

print("Content-Type: text/html")    # HTML is following
print()                             # blank line, end of headers

print("<html><head><title>CGI Test</title></head>")
print("<body>")
print("<h1>Hello World!</h1>")
print("</body></html>")

#!/usr/bin/env python
# hello.py

# Enable debugging
import cgitb
cgitb.enable()

print('''
<!DOCTYPE html>
<html>
<head>
	<title>CGI Test</title>
</head>
<body>
	<h1>Hello World!</h1>
</body>
</html>
	''')

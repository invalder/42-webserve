import os
import sys

def main():
	''' CGI for create a file

	Exit status code:
		0: complete
		2: invalid method
	'''
	# get request method from environment variable
	method = os.environ['REQUEST_METHOD']

	# if method is not DELETE, raise exception to webserver
	if method != 'POST':
		# exit with status code 2 if method is not DELETE
		sys.exit(2)

	# get file path from environment variable
	filePath = os.environ['FILE_PATH']

	with open(filePath, 'w') as f:
		# get file body from environment variable
		fileBody = os.environ['BODY']

		# write file body to file
		f.write( fileBody )

	print( 'COMPLETE' )


if __name__ == '__main__':
	main()
import os
import sys

def main():
	''' CGI for delete a file.
		This CGI does not check if file exists because webserver will check it.

	Exit status code:
		0: complete
		1: file does not exist
		2: invalid method
	'''
	# get request method from environment variable
	method = os.environ['REQUEST_METHOD']

	# if method is not DELETE, raise exception to webserver
	if method != 'DELETE':
		# exit with status code 2 if method is not DELETE
		sys.exit(2)

	# get file path from environment variable
	filePath = os.environ['FILE_PATH']

	# try to delete file
	try:
		os.remove( filePath )
	except:
		# exit with status code 1 if file does not exist
		sys.exit(1)

	print( 'DELETE COMPLETE' )


if __name__ == '__main__':
	main()
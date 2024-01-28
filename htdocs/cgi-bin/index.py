import os
import sys
import urllib.parse

# get working directory of this file
FILE_PATH = os.path.dirname(os.path.realpath(__file__))

# get working directory of webserver
WORKING_DIR = os.path.dirname(FILE_PATH)

def getPath( directoryName: str ) -> str:
	''' helper to get path of given directory name
	'''
	for root, dir, files in os.walk(WORKING_DIR):
		if directoryName in dir:
			return os.path.join(root, directoryName)
	return ''

def parseUrl( url: str ):
	''' helper to parse url and return dict
	'''
	# parse url
	parsedUrl = urllib.parse.urlparse(url)

	# get query string
	queryString = parsedUrl.query

	# parse query string
	queryString = urllib.parse.parse_qs(queryString)

	return queryString

def constructResponse( statusCode: int, statusMessage: str, contentType: str, contentLength: int, fileName: str = '', body: str = '' ) -> str:
	''' helper to construct response
	'''
	response = f'HTTP/1.1 {statusCode} {statusMessage}\r\n'
	response += f'Content-Type: {contentType}\r\n'
	response += f'Content-Length: {contentLength}\r\n'
	response += f'Content-Disposition: attachment; filename="{fileName}"\r\n' if fileName else ''
	response += 'Connection: close\r\n'
	response += '\r\n'
	response += body

	return response

def handlePost( filePath: str):
	''' helper to handle POST request
	'''
	with open(filePath, 'w') as f:
		# get file body from environment variable
		fileBody = os.environ['BODY']

		# write file body to file
		f.write( fileBody )

		# get file size
		fileSize = f.tell()

		# construct response
		response = constructResponse( 201, 'Created', 'text/plain', fileSize )

		# print response to webserver
		print(response)

		# exit with status code 0
		sys.exit(0)

def handleDelete( filePath: str ):
	''' helper to handle DELETE request
	'''
	# check if file exists
	if not os.path.exists(filePath):
		print('Cannot remove file: File does not exist')
		sys.exit(3)

	# check if file is writable
	if not os.access(filePath, os.W_OK):
		print('Cannot remove file: File is not writable')
		sys.exit(4)

	# try to remove file
	try:
		os.remove(filePath)

		# construct response
		response = constructResponse( 200, 'OK', 'text/plain', 0 )

		# print response to webserver
		print(response)

		# exit with status code 0
		sys.exit(0)

	except Exception as e:
		print(f"Error: {e}")
		raise

def handleGet( filePath: str, fileName: str ):
	# check if file exists
	if not os.path.exists(filePath):
		print('Cannot read file: File does not exist')
		sys.exit(3)

	# check if file is readable
	if not os.access(filePath, os.R_OK):
		print('Cannot read file: File is not readable')
		sys.exit(4)

	# try to read file and return to webserver
	try:
		fileBody = ''
		with open(filePath, 'r') as f:
			# read file
			fileBody = f.read()

			response = constructResponse( 200, 'OK', 'text/plain', len(fileBody), fileName, fileBody )

			# print response to webserver
			print(response)

			# exit with status code 0
			sys.exit(0)

	except Exception as e:
		print(f"Error: {e}")
		raise


def main():
	''' CGI for create a file

	Exit status code:
		0: complete
		2: invalid method
	'''
	reqeustUri = os.environ['REQUEST_URI']
	# print( '#' * 50 )
	# print( reqeustUri )
	# get path and query string from environment variable
	queryString = parseUrl( reqeustUri )

	# print( queryString )
	# print( '#' * 50 )

	fileName = queryString['file'][0]

	# get file path from environment variable
	uploadPath = getPath('upload')

	# create file path
	filePath = os.path.join(uploadPath, fileName)
	# get request method from environment variable

	method = os.environ['REQUEST_METHOD']

	# if method is not DELETE, raise exception to webserver
	if method == 'POST':
		handlePost( filePath )

	elif method == 'DELETE':
		handleDelete( filePath )

	elif method == 'GET':
		handleGet( filePath, fileName )

	# exit with status code 2 if method is not DELETE
	sys.exit(2)


if __name__ == '__main__':
	main()
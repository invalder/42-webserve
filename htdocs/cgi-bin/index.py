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

def parseUrl( url: str ) -> tuple[str, dict]:
	''' helper to parse url and return dict
	'''
	# parse url
	parsedUrl = urllib.parse.urlparse(url)

	# get query string
	queryString = parsedUrl.query

	# parse query string
	queryString = urllib.parse.parse_qs(queryString)

	return queryString

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

		with open(filePath, 'w') as f:
			# get file body from environment variable
			fileBody = os.environ['BODY']

			# write file body to file
			f.write( fileBody )

	elif method == 'DELETE':

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
		except Exception as e:
			print(f"Error: {e}")
			raise

	elif method == 'GET':
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

				# construct response
				response = 'HTTP/1.1 200 OK\r\n'
				response += 'Content-Type: text/plain\n'
				response += f'Content-Length: {len(fileBody)}\r\n'
				response += f'Content-Disposition: attachment; filename="{fileName}"\r\n'
				response += 'Connection: close\r\n'
				response += '\r\n'

				# print response to webserver
				print(response + fileBody)

		except Exception as e:
			print(f"Error: {e}")
			raise

	# exit with status code 2 if method is not DELETE
	sys.exit(2)


if __name__ == '__main__':
	main()
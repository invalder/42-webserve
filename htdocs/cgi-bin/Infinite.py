from time import sleep

def main():
	i = 0
	while True:
		print( f'Sleep for {i} seconds')
		i += 1
		sleep( 1 )
	
if __name__ == '__main__':
	main()
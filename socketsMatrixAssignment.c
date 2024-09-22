#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define ROW_COLUMN_COUNT 3

void Inject(u_int8_t* arr, int offset, int value)
{
	u_int8_t _byte;
	offset *= 4;

	_byte = value >> 24;
	arr[offset++] = _byte;

	_byte = value >> 16;
	arr[offset++] = _byte;

	_byte = value >> 8;
	arr[offset++] = _byte;

	_byte = value;
	arr[offset++] = _byte;
}

void printMatrix(int Matrix[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT]) {
	int i,j;
	for (i = 0; i < ROW_COLUMN_COUNT; i++)
	{
		for (j = 0; j < ROW_COLUMN_COUNT; j++)
		{
			printf("%d ", Matrix[i][j]);
		}
		printf("\n");
	}
}


void deserialize(u_int8_t* buf, int n, int Matrix[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT] )
{
	int ee = 0;
	int rr = 0;
	int sum;
	int i;
	for(i = 0; i < n/4; i++) {
		sum = 0;
		sum |= buf[i*4+0] << 24;
		sum |= buf[i*4+1] << 16;
		sum |= buf[i*4+2] << 8;
		sum |= buf[i*4+3];
		Matrix[ee][rr++] = sum;

		if(rr == ROW_COLUMN_COUNT) {
			ee++;
			rr = 0;
		}
	}		
}

int main()
{
	int sockets[2]; //sockets
	int child;

	int MatrixA[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT];
	int MatrixB[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT];
	int MatrixResults[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT];

	int BuffHalfMatrix[ROW_COLUMN_COUNT - ROW_COLUMN_COUNT/2][ROW_COLUMN_COUNT];
	int BuffMatrixResults[ROW_COLUMN_COUNT][ROW_COLUMN_COUNT];
	
	int row, col, i, k;
	int sum = 0;


	//create socket pair for parent and child
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
	{
		perror("opening stream socket pair");
		exit(1);
	}

	//Generate Matricies A and B
	srand(time_t(NULL));
	for (row = 0; row < ROW_COLUMN_COUNT; row++)
	{
		for (col = 0; col < ROW_COLUMN_COUNT; col++)
		{
			MatrixA[row][col] = rand() / 100000000;
			MatrixB[row][col] = rand() / 100000000;
			MatrixResults[row][col] = 0;
			sleep(0);
		}
	}

	//Print Generated Matricies
	printf("\nMatrix A-->\n");
	printMatrix(MatrixA);
	
	printf("\n\nMatrix B-->\n");
	printMatrix(MatrixB);

	// Create Child Process
	child = fork();
	if (child == -1)	perror("fork");
	else if (child)
	{ /************** This is the parent. **************/
		u_int8_t buf[(ROW_COLUMN_COUNT - ROW_COLUMN_COUNT/2)*(ROW_COLUMN_COUNT)*4];

		//TODO: Calculate the first half of the multiplication here
		for(row = 0; row < ROW_COLUMN_COUNT/2; row++) 
		{
			for(col = 0; col < ROW_COLUMN_COUNT; col++) 
			{
				sum = 0;
				for(k = 0; k < ROW_COLUMN_COUNT; k++) 
				{
					sum += MatrixA[row][k] * MatrixB[k][col];
				}
				MatrixResults[row][col] = sum;
			}
		}

		//TODO: Read the results from the child process of the other half
		read(sockets[1], buf, sizeof(buf));

		// deserialize received data to get results
		deserialize(buf, sizeof(buf), BuffHalfMatrix);

		//Merge Results
		k = 0;
		for (row = ROW_COLUMN_COUNT/2; row < ROW_COLUMN_COUNT; row++)
		{
			for (col = 0; col < ROW_COLUMN_COUNT; col++)
			{
				MatrixResults[row][col] = BuffHalfMatrix[k][col];
			}
			k++;
		}

		//Print Results in Parent
		u_int8_t bufResult[ROW_COLUMN_COUNT*ROW_COLUMN_COUNT*4];
		int offset = 0;
		printf("\n\nMatrixResults-->\n");
		for (row = 0; row < ROW_COLUMN_COUNT; row++)
		{
			for (col = 0; col < ROW_COLUMN_COUNT; col++)
			{
				printf("%d ", MatrixResults[row][col]);
				Inject(bufResult, offset++, MatrixResults[row][col]);
			}
			printf("\n");
		}
		printf("\n\n");
		
		//TODO: Send Results to Child
		send(sockets[1],bufResult,sizeof(bufResult),0);

		close(sockets[1]);
	}
	else
	{ /*****************  This is the child. ***************/
		u_int8_t bufToSend[(ROW_COLUMN_COUNT - ROW_COLUMN_COUNT/2)*(ROW_COLUMN_COUNT)*4];
		int offset = 0;
		
		//TODO: Calculate Results for the other half in child process
		for(row = ROW_COLUMN_COUNT/2; row < ROW_COLUMN_COUNT; row++) 
		{
			for(col = 0; col < ROW_COLUMN_COUNT; col++) 
			{
				sum = 0;
				for(k = 0; k < ROW_COLUMN_COUNT; k++) 
				{
					sum += MatrixA[row][k] * MatrixB[k][col];
				}
				Inject(bufToSend, offset++, sum);
			}
		}
		
		//TODO: Send it back to parent
		send(sockets[0], bufToSend,sizeof(bufToSend), 0);

		//TODO: Read whole results here to print
		u_int8_t bufToReceive[ROW_COLUMN_COUNT*ROW_COLUMN_COUNT*4];
		recv(sockets[0], bufToReceive, sizeof(bufToReceive),0);

		// deserialize received data to get results
		deserialize(bufToReceive,sizeof(bufToReceive),BuffMatrixResults);
	
		//Print Results in Child
		printf("MatrixResults at child-->\n");
		printMatrix(BuffMatrixResults);
		
		close(sockets[0]);
	}
}

#include <stdio.h>

char buf[1024];

int main(int argc ,char **argv )
{
	FILE *y_in;
	FILE *uv_in;
	FILE *yuv_out;
	int s;
	int i = 0;
	char input;
	
	if (argc != 4)
	{
		printf("usage combiner <y_inout> <uv_input> <yuv_output>\n");
		return -1;
	}
	printf("arg1=%s\n",argv[1]);
	printf("arg2=%s\n",argv[2]);
	printf("arg3=%s\n",argv[3]);
	
	y_in = fopen(argv[1], "rb");
	uv_in = fopen(argv[2], "rb");
	yuv_out = fopen(argv[3], "wb");

	if ( y_in == NULL || uv_in == NULL || yuv_out == NULL) {
		printf("Error: can't open file\n");
		return -1;
	}

	// copy y_in to yuv_out
	while (!feof(y_in)) {
		s = fread(buf, 1, sizeof(buf), y_in);
		fwrite(buf, 1, s, yuv_out);
	}

	fclose(y_in);

	// copy only u from uv_in
	while(!feof(uv_in)) {
		if (i%2 == 0)	{
			fread(&input, 1, 1, uv_in);
			fwrite(&input, 1, 1, yuv_out);
		}
		else {
			fread(&input, 1, 1, uv_in);
		}
		
		i++;
	}

	fseek( uv_in , 0, SEEK_SET);
	i = 0;
	
	while(!feof(uv_in)) {
		if (i%2 == 1)	{
			fread(&input, 1, 1, uv_in);
			fwrite(&input, 1, 1, yuv_out);
		}
		else {
			fread(&input, 1, 1, uv_in);
		}

		i++;
	}


	fclose(uv_in);
	fclose(yuv_out);

}

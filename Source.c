// SPDX-License-Identifier: LicenseRef-PurposeSource-1.0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Color
{
	unsigned char R, G, B;
};
struct Size
{
	int Dimension, Width, Height, Padding;
};
struct Bitmap
{
	struct Size size;
	struct Color* Map;
	char *Header;
};
void Initialize(char *adress, struct Bitmap *image)
{
	FILE *source;
	source = fopen(adress, "rb");
	if (source == NULL)
	{
		printf("Can't find the source image");
		return;
	}
	fseek(source, 2, SEEK_SET);
	fread(&image->size.Dimension, sizeof(unsigned int), 1, source);
	fseek(source, 18, SEEK_SET);
	fread(&image->size.Width, sizeof(unsigned int), 1, source);
	fread(&image->size.Height, sizeof(unsigned int), 1, source);
	int padding;
	if (image->size.Width % 4 != 0)
		padding = 4 - (3 * image->size.Width) % 4;
	else
		padding = 0;
	image->size.Padding = padding;
	image->Map = malloc(sizeof(struct Color)*image->size.Width*image->size.Height);
	fseek(source, 0, SEEK_SET);
	image->Header = malloc(54);
	fread(image->Header, 1, 54, source);
	int i, j;
	unsigned char c;
	unsigned char* rezidual = malloc(4);
	fseek(source, 54, SEEK_SET);
	for (i = 0; i < image->size.Height; i++)
	{
		for (j = 0; j < image->size.Width; j++)
		{
			fread(&c, 1, 1, source);
			image->Map[i + j * (image->size.Height)].B = c;
			fread(&c, 1, 1, source);
			image->Map[i + j * (image->size.Height)].G = c;
			fread(&c, 1, 1, source);
			image->Map[i + j * (image->size.Height)].R = c;
		}
		if (image->size.Padding != 0)
			fread(rezidual, 1, image->size.Padding, source);
	}
	fclose(source);
}
void Write(char *adress, struct Bitmap *image)
{
	FILE *destination;
	destination = fopen(adress, "w");
	if (destination == NULL)
	{
		printf("Can't find the file that will be overwrited");
		return;
	}
	int i, j;
	unsigned char* rezidual = calloc(1, 4);
	fwrite(image->Header, 1, 54, destination);
	for (i = 0; i < image->size.Height; i++)
	{
		for (j = 0; j < image->size.Width; j++)
		{
			fwrite(&image->Map[i + j * (image->size.Height)].B, 1, 1, destination);
			fwrite(&image->Map[i + j * (image->size.Height)].G, 1, 1, destination);
			fwrite(&image->Map[i + j * (image->size.Height)].R, 1, 1, destination);
		}
		if (image->size.Padding != 0)
			fwrite(rezidual, 1, image->size.Padding, destination);
	}
	fclose(destination);
}
struct Color XOR_Pixel_Pixel(struct Color P, struct Color R)
{
	struct Color Q;
	Q.R = P.R^R.R;
	Q.G = P.G^R.G;
	Q.B = P.B^R.B;
	return Q;
}
struct Color XOR_Pixel_Int(struct Color P, int X)
{
	struct Color Q;
	char* C = &X;
	Q.R = P.R^C[1];
	Q.G = P.G^C[2];
	Q.B = P.B^C[3];
	return Q;
}
unsigned int xorshift32(unsigned int *state)
{
	unsigned int x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}
void CopyImage(struct Bitmap *from, struct Bitmap *to, int IncludeContent)
{
	to->size = from->size;
	int i;
	to->Header = malloc(54);
	for (i = 0; i < 54; i++)
	{
		to->Header[i] = from->Header[i];
	}
	to->Map = malloc(sizeof(struct Color)*from->size.Width*(from->size.Height));
	if (IncludeContent)
	{
		int j;
		for (i = 0; i < from->size.Width; i++)
		{
			for (j = 0; j < from->size.Height; j++)
			{
				to->Map[i*(from->size.Height) + j].B = from->Map[i*(from->size.Height) + j].B;
				to->Map[i*(from->size.Height) + j].G = from->Map[i*(from->size.Height) + j].G;
				to->Map[i*(from->size.Height) + j].R = from->Map[i*(from->size.Height) + j].R;
			}
		}
	}
}
void Encrypt(char* originalAdress, char* encryptedAdress, char* keyAdress)
{
	struct Bitmap image;
	Initialize(originalAdress, &image);
	FILE* Key = fopen(keyAdress, "r");
	unsigned int R0, SV, buf;
	fscanf(Key, "%u %u", &R0, &SV);
	unsigned int *R = malloc(sizeof(unsigned int)*(image.size.Width*image.size.Height * 2 - 1));
	buf = R0;
	R[0] = buf;
	unsigned int i;
	for (i = 0; i < image.size.Width * image.size.Height * 2 - 1; i++)
	{
		xorshift32(&buf);
		R[i] = buf;
	}
	unsigned int *Sigma = malloc(sizeof(unsigned int)*image.size.Width*image.size.Height);
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		Sigma[i] = i;
	}
	for (i = image.size.Width*image.size.Height - 1; i > 0; i--)
	{
		unsigned int j = R[i] % (i + 1);
		unsigned int buffer = Sigma[i];
		Sigma[i] = Sigma[j];
		Sigma[j] = buffer;
	}
	struct Bitmap Pp;
	CopyImage(&image, &Pp, 0);
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		Pp.Map[Sigma[i]] = image.Map[i];
	}
	Pp.Map[0] = XOR_Pixel_Int(XOR_Pixel_Int(Pp.Map[0], SV), R[image.size.Width*image.size.Height - 1]);
	for (i = 1; i < image.size.Width*image.size.Height; i++)
	{
		Pp.Map[i] = XOR_Pixel_Int(XOR_Pixel_Pixel(Pp.Map[i - 1], Pp.Map[i]), R[image.size.Width*image.size.Height + i - 1]);
	}
	CopyImage(&Pp, &image, 1);
	Write(encryptedAdress, &image);
}

void Decrypt(char* originalAdress, char* encryptedAdress, char* keyAdress)
{
	struct Bitmap image;
	Initialize(encryptedAdress, &image);
	FILE* Key = fopen(keyAdress, "r");
	unsigned int R0, SV, buf;
	fscanf(Key, "%u %u", &R0, &SV);
	unsigned int *R = malloc(sizeof(unsigned int)*(image.size.Width*image.size.Height * 2 - 1));
	buf = R0;
	R[0] = buf;
	unsigned int i;
	for (i = 0; i < image.size.Width * image.size.Height * 2 - 1; i++)
	{
		xorshift32(&buf);
		R[i] = buf;
	}
	unsigned int *Sigma = malloc(sizeof(unsigned int)*image.size.Width*image.size.Height);
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		Sigma[i] = i;
	}
	for (i = image.size.Width*image.size.Height - 1; i > 0; i--)
	{
		unsigned int j = R[i] % (i + 1);
		unsigned int buffer = Sigma[i];
		Sigma[i] = Sigma[j];
		Sigma[j] = buffer;
	}
	unsigned int *SigmaReverted = malloc(sizeof(unsigned int)*image.size.Width*image.size.Height);
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		SigmaReverted[Sigma[i]] = i;
	}
	struct Bitmap Pp;
	CopyImage(&image, &Pp, 1);
	for (i = image.size.Width*image.size.Height - 1; i > 0; i--)
	{
		Pp.Map[i] = XOR_Pixel_Int(XOR_Pixel_Pixel(Pp.Map[i - 1], Pp.Map[i]), R[image.size.Width*image.size.Height + i - 1]);
	}
	Pp.Map[0] = XOR_Pixel_Int(XOR_Pixel_Int(Pp.Map[0], SV), R[image.size.Width*image.size.Height - 1]);
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		image.Map[SigmaReverted[i]] = Pp.Map[i];
	}
	Write(originalAdress, &image);
}
void XiSquaredTest(char* location)
{
	struct Bitmap image;
	Initialize(location, &image);
	float FBarred = image.size.Width*image.size.Height / 256;
	int **Frequency;
	float *XiSquared;
	int i, j;
	Frequency = malloc(sizeof(int*) * 3);
	XiSquared = malloc(sizeof(float) * 3);
	for (i = 0; i < 3; i++)
	{
		Frequency[i] = malloc(sizeof(int) * 256);
		XiSquared[i] = 0;
		for (j = 0; j < 256; j++)
			Frequency[i][j] = 0;
	}
	for (i = 0; i < image.size.Width*image.size.Height; i++)
	{
		Frequency[0][image.Map[i].R]++;
		Frequency[1][image.Map[i].G]++;
		Frequency[2][image.Map[i].B]++;
	}
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 256; j++)
		{
			XiSquared[i] += (Frequency[i][j] - FBarred)*(Frequency[i][j] - FBarred) / FBarred;
		}
	}
	printf("Red resoult: %f\nGreen resoult: %f\nBlue resoult: %f\n", XiSquared[0], XiSquared[1], XiSquared[2]);
}
void main()
{
	char *pathInputImage, *pathOutputImage, *pathKeysFile;
	printf("Do you want to encrypt or to decrypt a picture?\n1-Encrypt\n2-Decrypt\n->");
	int choice;
	scanf("%d", &choice);
	if (choice == 1)
	{
		//load input file path
		printf("Insert the path of the picture to be encrypted ");
		pathInputImage = malloc(200);
		scanf("%s", pathInputImage);
		//load output file path
		printf("Insert the path of the picture to be overwrited with the encrypted image: ");
		pathOutputImage = malloc(200);
		scanf("%s", pathOutputImage);
		//load keys path
		printf("Insert the path of the file with the encryption keys: ");
		pathKeysFile = malloc(200);
		scanf("%s", pathKeysFile);
		//Make the encryption
		Encrypt(pathInputImage, pathOutputImage, pathKeysFile);
		//XiSquared Test
		printf("The XiSquared Test for an image represents it's \"clarity\".\n The test for the original image:\n");
		XiSquaredTest(pathInputImage);
		printf("and for the encrypted image:\n");
		XiSquaredTest(pathOutputImage);
		scanf("%s", pathKeysFile);
		
	}
	else
	{
		//load input file path
		printf("Insert the path of the picture to be decrypted ");
		pathInputImage = malloc(200);
		scanf("%s", pathInputImage);
		//load output file path
		printf("Insert the path of the picture to be overwrited with the decrypted image: ");
		pathOutputImage = malloc(200);
		scanf("%s", pathOutputImage);
		//load keys path
		printf("Insert the path of the file with the decryption keys: ");
		pathKeysFile = malloc(200);
		scanf("%s", pathKeysFile);
		//Make the decryption
		Decrypt(pathOutputImage, pathInputImage, pathKeysFile);
		//XiSquared Test
		printf("The XiSquared Test for an image represents it's \"clarity\".\n The test for the encrypted image:\n");
		XiSquaredTest(pathInputImage);
		printf("and for the decrypted image:\n");
		XiSquaredTest(pathOutputImage);
	}
}
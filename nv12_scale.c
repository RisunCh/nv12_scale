#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

typedef unsigned char uint8_t;
 
/** 
 * @param src input nv12 raw data array  
 * @param dst output nv12 raw data result, 
 * the memory need to be allocated outside of the function 
 * @param srcWidth width of the input nv12 image 
 * @param srcHeight height of the input nv12 image 
 * @param dstWidth
 * @param dstHeight 
 */

void nv12_nearest_scale(uint8_t* __restrict src, uint8_t* __restrict dst,
                        int srcWidth, int srcHeight, int dstWidth, int
dstHeight)      //restrict keyword is for compiler to optimize program
{
    register int sw = srcWidth;  //register keyword is for local var to accelorate 
    register int sh = srcHeight;
    register int dw = dstWidth;
    register int dh = dstHeight;
    register int y, x;
    unsigned long int srcy, srcx, src_index, dst_index;
    unsigned long int xrIntFloat_16 = (sw << 16) / dw + 1; //better than float division
    unsigned long int yrIntFloat_16 = (sh << 16) / dh + 1;

    uint8_t* dst_uv = dst + dh * dw; //memory start pointer of dest uv
    uint8_t* src_uv = src + sh * sw; //memory start pointer of source uv
    uint8_t* dst_uv_yScanline;
    uint8_t* src_uv_yScanline;
    uint8_t* dst_y_slice = dst; //memory start pointer of dest y
    uint8_t* src_y_slice;
    uint8_t* sp;
    uint8_t* dp;
 
    for (y = 0; y < (dh & ~7); ++y)  
    {
        srcy = (y * yrIntFloat_16) >> 16;
        src_y_slice = src + srcy * sw;

        if((y & 1) == 0)
        {
            dst_uv_yScanline = dst_uv + (y / 2) * dw;
            src_uv_yScanline = src_uv + (srcy / 2) * sw;
        }

        for(x = 0; x < (dw & ~7); ++x)
        {
            srcx = (x * xrIntFloat_16) >> 16;
            dst_y_slice[x] = src_y_slice[srcx];

            if((y & 1) == 0) //y is even
	          {
	              if((x & 1) == 0) //x is even
                {
		                src_index = (srcx / 2) * 2;
			
                    sp = dst_uv_yScanline + x;
                    dp = src_uv_yScanline + src_index;
                    *sp = *dp;
                    ++sp;
                    ++dp;
                    *sp = *dp;
                }
	          }
        }
        dst_y_slice += dw;
    }
}

void nv12_bilinear_scale (uint8_t* src, uint8_t* dst,
		int srcWidth, int srcHeight, int dstWidth,int dstHeight)
{
    int x, y;
    int ox, oy;
    int tmpx, tmpy;
    int xratio = (srcWidth << 8)/dstWidth;
    int yratio = (srcHeight << 8)/dstHeight;
    uint8_t* dst_y = dst;
    uint8_t* dst_uv = dst + dstHeight * dstWidth;
    uint8_t* src_y = src;
    uint8_t* src_uv = src + srcHeight * srcWidth;

    uint8_t y_plane_color[2][2];
    uint8_t u_plane_color[2][2];
    uint8_t v_plane_color[2][2];
    int j,i;
    int size = srcWidth * srcHeight;
    int offsetY;
    int y_final, u_final, v_final; 
    int u_final1 = 0;
    int v_final1 = 0;
    int u_final2 = 0;
    int v_final2 = 0;
    int u_final3 = 0;
    int v_final3 = 0;
    int u_final4 = 0;
    int v_final4 = 0;
    int u_sum = 0;
    int v_sum = 0;


    tmpy = 0;
    for (j = 0; j < (dstHeight & ~7); ++j)
    {
        //tmpy = j * yratio;
	      oy = tmpy >> 8;
	      y = tmpy & 0xFF;

	      tmpx = 0;
	      for (i = 0; i < (dstWidth & ~7); ++i)
	      {
	        // tmpx = i * xratio;
	        ox = tmpx >> 8;
	        x = tmpx & 0xFF;
	
	        offsetY = oy * srcWidth;
          //YYYYYYYYYYYYYYYY
	        y_plane_color[0][0] = src[ offsetY + ox ];
	        y_plane_color[1][0] = src[ offsetY + ox + 1 ];
	        y_plane_color[0][1] = src[ offsetY + srcWidth + ox ];
	        y_plane_color[1][1] = src[ offsetY + srcWidth + ox + 1 ];
			
	        int y_final = (0x100 - x) * (0x100 - y) * y_plane_color[0][0]
			                  + x * (0x100 - y) * y_plane_color[1][0]
			                  + (0x100 - x) * y * y_plane_color[0][1]
			                  + x * y * y_plane_color[1][1];
	        y_final = y_final >> 16;
	        if (y_final>255)
		        y_final = 255;
          if (y_final<0)
	           y_final = 0;
          dst_y[ j * dstWidth + i] = (uint8_t)y_final; //set Y in dest array 
          //UVUVUVUVUVUV
	        if((j & 1) == 0) //j is even
	        {
	            if((i & 1) == 0) //i is even
		          {
	  	            u_plane_color[0][0] = src[ size + offsetY + ox ];
        	        u_plane_color[1][0] = src[ size + offsetY + ox ];
	                u_plane_color[0][1] = src[ size + offsetY + ox ];
                  u_plane_color[1][1] = src[ size + offsetY + ox ];

                  v_plane_color[0][0] = src[ size + offsetY + ox + 1];
                  v_plane_color[1][0] = src[ size + offsetY + ox + 1];
                  v_plane_color[0][1] = src[ size + offsetY + ox + 1];
                  v_plane_color[1][1] = src[ size + offsetY + ox + 1];
		          }
		          else //i is odd
		          {
                  u_plane_color[0][0] = src[ size + offsetY + ox - 1 ];
                  u_plane_color[1][0] = src[ size + offsetY + ox + 1 ];
                  u_plane_color[0][1] = src[ size + offsetY + ox - 1 ];
                  u_plane_color[1][1] = src[ size + offsetY + ox + 1 ];

		              v_plane_color[0][0] = src[ size + offsetY + ox ];
                  v_plane_color[1][0] = src[ size + offsetY + ox + 1 ];
                  v_plane_color[0][1] = src[ size + offsetY + ox ];
                  v_plane_color[1][1] = src[ size + offsetY + ox + 1 ];
		          }
	
	        }
          else // j is odd
	        {
              if((i & 1) == 0) //i is even
              {
                  u_plane_color[0][0] = src[ size + offsetY + ox ];
                  u_plane_color[1][0] = src[ size + offsetY + ox ];
                  u_plane_color[0][1] = src[ size + offsetY + srcWidth + ox ];
                  u_plane_color[1][1] = src[ size + offsetY + srcWidth + ox ];
					
                  v_plane_color[0][0] = src[ size + offsetY + ox + 1];
                  v_plane_color[1][0] = src[ size + offsetY + ox + 1];
                  v_plane_color[0][1] = src[ size + offsetY + srcWidth + ox + 1];
                  v_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 1];			                                        
              }
              else //i is odd
              {
                  u_plane_color[0][0] = src[ size + offsetY + ox - 1 ];
                  u_plane_color[1][0] = src[ size + offsetY + srcWidth + ox - 1 ];
                  u_plane_color[0][1] = src[ size + offsetY + ox + 1];
                  u_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 1];

                  v_plane_color[0][0] = src[ size + offsetY + ox ];
                  v_plane_color[1][0] = src[ size + offsetY + srcWidth + ox ];
                  v_plane_color[0][1] = src[ size + offsetY + ox + 2 ];
                  v_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 2 ];
	           }
          }

          int u_final = (0x100 - x) * (0x100 - y) * u_plane_color[0][0]
                         + x * (0x100 - y) * u_plane_color[1][0]
                         + (0x100 - x) * y * u_plane_color[0][1]
                         + x * y * u_plane_color[1][1];
          u_final = u_final >> 16;

          int v_final = (0x100 - x) * (0x100 - y) * v_plane_color[0][0]
                          + x * (0x100 - y) * v_plane_color[1][0]
                          + (0x100 - x) * y * v_plane_color[0][1]
                          + x * y * v_plane_color[1][1];
          v_final = v_final >> 16;
          if((j & 1) == 0)
          {
              if((i & 1) == 0)
              {	
	                //set U in dest array  
                  dst_uv[(j / 2) * dstWidth + i ] = (uint8_t)(u_sum / 4);
                  //set V in dest array
                  dst_uv[(j / 2) * dstWidth + i + 1] = (uint8_t)(v_sum / 4);
		              u_sum = 0;
                  v_sum = 0;
               }
           }
	         else
	         {
		          u_sum += u_final;
		          v_sum += v_final;
	         }
	         tmpx += xratio;
	      }
	      tmpy += yratio;
    }
}

int ImageResize(uint8_t * src, uint8_t* dst, int sw,
		int sh,int dw,int dh)
{
	if( (src == NULL) || (dst == NULL) || (0 == dw) || (0 == dh) ||
			(0 == sw) || (0 == sh))
	{
	    printf("params error\n");
	    return -1;
	}
  nv12_nearest_scale(src, dst, sw, sh, dw, dh);
	//nv12_bilinear_scale(src, dst, sw, sh, dw, dh);
	//greyscale(src, dst, sw, sh, dw, dh);
	return 0;
}

int main(int argc,char**argv)
{
	if(argc!=7)
	{
	    printf("Input Error!\n");
	    printf("Usage :  <Input NV12file> <Output NV12file> <sw><sh> <dw> <dh>");
  		return 0;
	}
 
	FILE *inputfp = NULL;
	FILE *outputfp = NULL;
 
	inputfp = fopen(argv[1], "rb");
	if (!inputfp)
	{
	    fprintf(stderr, "fopen failed for input file[%s]\n",argv[1]);
	    return -1;
	}
 
	outputfp = fopen(argv[2], "wb");
 
	if (!outputfp)
	{
	    fprintf(stderr, "fopen failed for output file[%s]\n",argv[2]);
	    return -1;
	}
 
	int sw = atoi(argv[3]);
	int sh = atoi(argv[4]);
	int dw = atoi(argv[5]);
	int dh = atoi(argv[6]);
 
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <=0)
	{
	    fprintf(stderr, "parameter error [sw= %d,sh= %d,dw= %d,dh= %d]\n",sw,sh,dw,dh);
	    return -1;
	}
 
	int inPixels = sw * sh * 3/2;
	int outPixels = dw * dh * 3/2;
 
	uint8_t* pInBuffer = (uint8_t*)malloc(inPixels);
	fread(pInBuffer,1,inPixels,inputfp);
	uint8_t* pOutBuffer = (uint8_t*)malloc(outPixels);
 
	ImageResize(pInBuffer,pOutBuffer,sw,sh,dw,dh);
  //compute frame per second
	int i = 0;
	clock_t start = clock();
 
	for(;i<1000;++i)
	{
	    ImageResize(pInBuffer,pOutBuffer,1536,1088,1024,600);//can change to be any resolution	
  }
	clock_t finish = clock();
	float duration = (float)(finish-start)/CLOCKS_PER_SEC;
	float fps = 1000 / duration;
	printf("nv12Scaling:%d*%d-->%d*%d,time cost:%6.2ffps\n",sw,sh,dw,dh,fps);
 
	fwrite(pOutBuffer, 1 , outPixels, outputfp);
 
	free(pInBuffer);
	free(pOutBuffer);
	fclose(inputfp);
	fclose(outputfp);
	pInBuffer = NULL;
	pOutBuffer = NULL;
	inputfp = NULL;
	outputfp = NULL;
	return 0;
}

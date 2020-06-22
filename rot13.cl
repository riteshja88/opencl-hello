__kernel
void rot13(__global const char* in,
		   __global char* out)
{							
    const uint index = get_global_id(0);		
							
    char c = in[index];					
    if (c < 'A' || c > 'z' || (c > 'Z' && c < 'a')) {		
        out[index] = in[index];				
    } else {						
        if (c > 'm' || (c > 'M' && c < 'a')) {		
			out[index] = in[index] - 13;			
		} else {					
			out[index] = in[index] + 13;			
		}						
    }							
}

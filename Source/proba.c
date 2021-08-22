#include <stdio.h>

#define OUT 0
#define FIRST 1
#define SECOND 2
#define FLAG 0

int hex2(c) {
        if (c >= '0' && c <= '9') return c-'0';
        if (c >= 'A' && c <= 'F') return c-'A'+10;
        if (c >= 'a' && c <= 'f') return c-'a'+10;
        return -1;
}

main() { 
	int i;

	if(FLAG)
		for(i=0;i<13;i++)
			getchar();
	asc_to_nib();
} /* main */

nib_to_asc() {
        int c;
        int num;

        while((c=getchar()) != EOF) {
        	num = c % 16;
                putchar(num);
        	num = ( c - ( c % 16 ) ) / 16;
                putchar(num);
	} /* while */
} /* nib_to_asc */

asc_to_nib() {
        int c, state = FIRST;
        int num;

        while((c=getchar()) != EOF) {
                switch(state) {
                case OUT:
                        if (c==':') state=FIRST;
                        break;
                case FIRST:
                        num = (c);
                        if (num<0)
                                state = OUT;
                        else
                                state = SECOND;
                        break;
                case SECOND:
#if 0
                        num = num * 16 + (c);
#else
                        num = num + 16 * (c);
#endif
                        putchar(num);
                        state= FIRST;
                } /* switch */
        } /* while getchar */
} /* asc_to_nib */

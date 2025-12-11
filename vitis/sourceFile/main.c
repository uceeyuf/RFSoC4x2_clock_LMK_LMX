#include "LMK_LMX.h"
#include "stdio.h"
#include "sleep.h"
//#include "OLED.h"

int main(){
    printf("this is a test\n");


    write_clk(0);
    write_clk(0x01);    
    write_clk(0x02);   



    //write_clk(2);


    // LMK04828_write(0);
    // LMX2594_write(1);
    // LMX2594_write(2);


    

    //LMK_LMX();
    //write_OLED();
    while(1){

    }
    return 0; 
}
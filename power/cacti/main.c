/*------------------------------------------------------------
 *                              CACTI 4.0
 *         Copyright 2005 Hewlett-Packard Development Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein, and
 * hereby grant back to Hewlett-Packard Company and its affiliated companies ("HP")
 * a non-exclusive, unrestricted, royalty-free right and license under any changes, 
 * enhancements or extensions  made to the core functions of the software, including 
 * but not limited to those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to HP any such changes,
 * enhancements or extensions that they make and inform HP of noteworthy uses of
 * this software.  Correspondence should be provided to HP at:
 *
 *                       Director of Intellectual Property Licensing
 *                       Office of Strategy and Technology
 *                       Hewlett-Packard Company
 *                       1501 Page Mill Road
 *                       Palo Alto, California  94304
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND HP DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL HP 
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

#include "def.h"
#include "areadef.h"
#include "stdio.h"
#include "leakage.h"
#include "time.h"
#include "io.h"
#include <string.h>



/*------------------------------------------------------------------------------*/
      
int main(int argc,char *argv[])
{
    total_result_type result2;
	
	//Note that the following command line options are permitted:
	//C B A TECH Nbanks
    //C B A TECH RWP ERP EWP Nbanks
	//C B A RWP ERP EWP NSER Nbanks TECH OUTPUTWIDTH CUSTOMTAG TAGWIDTH ACCESSMODE PURESRAM
	//(CUSTOMTAG should be set to 1 or 0 depending on whether the tag width needs to be
	//changed (1) or not(0). TAGWIDTH IS the desired tag width if the tag width is being 
	//changed. ACCESSMODE = 0 for normal access, ACCESSMODE = 1 for sequential access,
	//ACCESSMODE = 2 for fast access. PURESRAM = 1 if the memory is a scratchpad memory,
	//PURESRAM = 0 if the memory is a cache

    if (input_data(argc,argv) == ERROR) exit(1);
	else{
		if (argc == 6){
			if (strcmp(argv[3],"FA") == 0) {
				result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), 0, 1, 0, 0, 0, atoi(argv[5]), atof(argv[4]), 64, 0, 0, 0, 0);
			} 
			else if (strcmp(argv[3],"DM") == 0){
					result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), 1, 1, 0, 0, 0, atoi(argv[5]), atof(argv[4]), 64, 0, 0, 0, 0);
				 }
				 else{
					 result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 1, 0, 0, 0, atoi(argv[5]), atof(argv[4]), 64, 0, 0, 0, 0);
				 }
		}
		else if (argc == 9){
				if (strcmp(argv[3],"FA") == 0) {
					result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), 0, 1, 0, 0, 0, atoi(argv[5]), atof(argv[4]), 64, 0, 0, 0, 0);
				} 
				else if (strcmp(argv[3],"DM") == 0){
						result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), 1, 1, 0, 0, 0, atoi(argv[5]), atof(argv[4]), 64, 0, 0, 0, 0);
					 }
					 else{
						result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), 0, atoi(argv[8]), atof(argv[4]), 64, 0, 0, 0, 0);
						 }
			   }
			  else {
					result2 = cacti_interface(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atof(argv[9]), atoi(argv[10]), atoi(argv[11]), atoi(argv[12]), atoi(argv[13]), atoi(argv[14]));
			  }
	}

	output_data(&result2.result,&result2.area,&result2.params);
}


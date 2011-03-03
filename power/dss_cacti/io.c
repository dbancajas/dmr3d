/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/


#include <stdio.h>
#include <math.h>
#include "def.h"

#define NEXTINT(a) skip(); scanf("%d",&(a));
#define NEXTFLOAT(a) skip(); scanf("%lf",&(a));
						
/*---------------------------------------------------------------*/


int input_data(argc,argv,parameters)
int argc; char *argv[];
parameter_type *parameters;
{
   int C,B,A;

   /* input parameters:
         C B A  */

   if (argc!=4) {
      printf("Cmd-line parameters: C B A\n");
      exit(0);
   }
   C = atoi(argv[1]);
   if ((C < 64)) {
       printf("Cache size must >=64\n");
       exit(0);
   }

   B = atoi(argv[2]);
   if ((B < 1)) {
       printf("Block size must >=1\n");
       exit(0);
   }

   A = atoi(argv[3]);
   if ((A < 1)) {
       printf("Associativity must >=1\n");
       exit(0);
   }

   parameters->cache_size = C;
   parameters->block_size = B;
   parameters->associativity = A;
   parameters->number_of_sets = C/(B*A);

   if (parameters->number_of_sets < 1) {
      printf("Less than one set...\n");
      exit(0);
   }
   
   return(OK);
}






void output_time_components(out_fd,A,result)
FILE *out_fd;
int A;
result_type *result;
{
   fprintf(out_fd, " decode_data (ns): %g\n",result->decoder_delay_data/1e-9);
   fprintf(out_fd, " wordline_data (ns): %g\n",result->wordline_delay_data/1e-9);
   fprintf(out_fd, " bitline_data (ns): %g\n",result->bitline_delay_data/1e-9);
   fprintf(out_fd, " sense_amp_data (ns): %g\n",result->sense_amp_delay_data/1e-9);
   fprintf(out_fd, " decode_tag (ns): %g\n",result->decoder_delay_tag/1e-9);
   fprintf(out_fd, " wordline_tag (ns): %g\n",result->wordline_delay_tag/1e-9);
   fprintf(out_fd, " bitline_tag (ns): %g\n",result->bitline_delay_tag/1e-9);
   fprintf(out_fd, " sense_amp_tag (ns): %g\n",result->sense_amp_delay_tag/1e-9);
   fprintf(out_fd, " compare (ns): %g\n",result->compare_part_delay/1e-9);
   if (A == 1)
      fprintf(out_fd, " valid signal driver (ns): %g\n",result->drive_valid_delay/1e-9);
   else {
      fprintf(out_fd, " mux driver (ns): %g\n",result->drive_mux_delay/1e-9);
      fprintf(out_fd, " sel inverter (ns): %g\n",result->selb_delay/1e-9);
   }
   fprintf(out_fd, " data output driver (ns): %g\n",result->data_output_delay/1e-9);
   fprintf(out_fd, " total data path (with output driver) (ns): %g\n",result->decoder_delay_data/1e-9+result->wordline_delay_data/1e-9+result->bitline_delay_data/1e-9+result->sense_amp_delay_data/1e-9);
   if (A==1)
      fprintf(out_fd, " total tag path is dm (ns): %g\n", result->decoder_delay_tag/1e-9+result->wordline_delay_tag/1e-9+result->bitline_delay_tag/1e-9+result->sense_amp_delay_tag/1e-9+result->compare_part_delay/1e-9+result->drive_valid_delay/1e-9);
   else 
      fprintf(out_fd, " total tag path is set assoc (ns): %g\n", result->decoder_delay_tag/1e-9+result->wordline_delay_tag/1e-9+result->bitline_delay_tag/1e-9+result->sense_amp_delay_tag/1e-9+result->compare_part_delay/1e-9+result->drive_mux_delay/1e-9+result->selb_delay/1e-9);
   fprintf(out_fd, " precharge time (ns): %g\n",result->precharge_delay/1e-9);
}


   
void output_data(out_fd,result,parameters)
FILE *out_fd;
result_type *result;
parameter_type *parameters;
{
   double datapath,tagpath;

   datapath = result->decoder_delay_data+result->wordline_delay_data+result->bitline_delay_data+result->sense_amp_delay_data+result->data_output_delay;
   if (parameters->associativity == 1) {
         tagpath = result->decoder_delay_tag+result->wordline_delay_tag+result->bitline_delay_tag+result->sense_amp_delay_tag+result->compare_part_delay+result->drive_valid_delay;
   } else {
         tagpath = result->decoder_delay_tag+result->wordline_delay_tag+result->bitline_delay_tag+result->sense_amp_delay_tag+result->compare_part_delay+result->drive_mux_delay+result->selb_delay;
   }

#  if OUTPUTTYPE == LONG
      fprintf(out_fd, "\nCache Parameters:\n");
      fprintf(out_fd, "  Size in bytes: %d\n",parameters->cache_size);
      fprintf(out_fd, "  Number of sets: %d\n",parameters->number_of_sets);
      fprintf(out_fd, "  Associativity: %d\n",parameters->associativity);
      fprintf(out_fd, "  Block Size (bytes): %d\n",parameters->block_size);

      fprintf(out_fd, "\nAccess Time: %g\n",result->access_time);
      fprintf(out_fd, "Cycle Time:  %g\n",result->cycle_time);
      fprintf(out_fd, "\nBest Ndwl (L1): %d\n",result->best_Ndwl);
      fprintf(out_fd, "Best Ndbl (L1): %d\n",result->best_Ndbl);
      fprintf(out_fd, "Best Nspd (L1): %d\n",result->best_Nspd);
      fprintf(out_fd, "Best Ntwl (L1): %d\n",result->best_Ntwl);
      fprintf(out_fd, "Best Ntbl (L1): %d\n",result->best_Ntbl);
      fprintf(out_fd, "Best Ntspd (L1): %d\n",result->best_Ntspd);
      fprintf(out_fd, "\nTime Components:\n");
      fprintf(out_fd, " data side (with Output driver) (ns): %g\n",datapath/1e-9);
      fprintf(out_fd, " tag side (ns): %g\n",tagpath/1e-9);
      output_time_components(out_fd,parameters->associativity,result);

#  else
      printf("%d %d %d  %d %d %d %d %d %d  %e %e %e %e  %e %e %e %e  %e %e %e %e  %e %e %e %e  %e %e\n",
               parameters->cache_size,
	       parameters->block_size,
	       parameters->associativity,
	       result->best_Ndwl,
	       result->best_Ndbl,
	       result->best_Nspd,
	       result->best_Ntwl,
	       result->best_Ntbl,
	       result->best_Ntspd,
	       result->access_time,
	       result->cycle_time,
               datapath,
               tagpath,
               result->decoder_delay_data,
               result->wordline_delay_data,
               result->bitline_delay_data,
               result->sense_amp_delay_data,
	       result->decoder_delay_tag,
               result->wordline_delay_tag,
               result->bitline_delay_tag,
               result->sense_amp_delay_tag,
               result->compare_part_delay,
               result->drive_mux_delay,
               result->selb_delay,
	       result->drive_valid_delay,
               result->data_output_delay,
	       result->precharge_delay);



# endif

}

							   

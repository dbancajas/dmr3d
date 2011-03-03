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





//  #include "simics/first.h"
// RCSID("$Id: cache_power.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h"


#include <math.h>
#include "power_defs.h"
#include "circuit_param.h"
#include "cacti_def.h"
#include "cacti_interface.h"
#include "area_param.h"
#include "area.h"
#include "cacti_time.h"
#include "leakage.h"
#include "basic_circuit.h"
#include "stdio.h"

//extern double calculate_area (area_type, double);

//v4.1: Earlier all the dimensions (length/width/thickness) of transistors and wires and
//all delays were calculated for the 0.8 micron process and then scaled to the input techology.
//Now all dimensions and delays are calculated directly for the input technology.

int force_tag, force_tag_size;

static area_t *area_obj = new area_t();

void cacti_time_t::reset_tag_device_widths() {

    circuit_param_t::cparam_Wtdecdrivep_second = 0.0;
    circuit_param_t::cparam_Wtdecdriven_second = 0.0;
    circuit_param_t::cparam_Wtdecdrivep_first = 0.0;
    circuit_param_t::cparam_Wtdecdriven_first = 0.0;
    circuit_param_t::cparam_Wtdec3to8n = 0.0;
    circuit_param_t::cparam_Wtdec3to8p = 0.0;
    circuit_param_t::cparam_WtdecNORn  = 0.0;
    circuit_param_t::cparam_WtdecNORp  = 0.0;
    circuit_param_t::cparam_Wtdecinvn  = 0.0;
    circuit_param_t::cparam_Wtdecinvp  = 0.0;
    circuit_param_t::cparam_WtwlDrvn = 0.0;
    circuit_param_t::cparam_WtwlDrvp = 0.0;

    circuit_param_t::cparam_Wtbitpreequ = 0.0;
    circuit_param_t::cparam_Wtiso = 0.0;
    circuit_param_t::cparam_Wtpch = 0.0;
    circuit_param_t::cparam_Wtiso = 0.0;
    circuit_param_t::cparam_WtsenseEn = 0.0;
    circuit_param_t::cparam_WtsenseN = 0.0;
    circuit_param_t::cparam_WtsenseP = 0.0;
    circuit_param_t::cparam_WtoBufN = 0.0;
    circuit_param_t::cparam_WtoBufP = 0.0;
    circuit_param_t::cparam_WtsPch = 0.0;

    circuit_param_t::cparam_WtpchDrvp = 0.0; circuit_param_t::cparam_WtpchDrvn = 0.0;
    circuit_param_t::cparam_WtisoDrvp = 0.0; circuit_param_t::cparam_WtisoDrvn = 0.0;
    circuit_param_t::cparam_WtspchDrvp = 0.0; circuit_param_t::cparam_WtspchDrvn = 0.0;
    circuit_param_t::cparam_WtsenseEnDrvp = 0.0; circuit_param_t::cparam_WtsenseEnDrvn = 0.0;

    circuit_param_t::cparam_WtwrtMuxSelDrvn = 0.0;
    circuit_param_t::cparam_WtwrtMuxSelDrvp = 0.0;
}
void cacti_time_t::reset_data_device_widths()
{
    circuit_param_t::cparam_Waddrdrvn1 = 0.0;
    circuit_param_t::cparam_Waddrdrvp1 = 0.0;
    circuit_param_t::cparam_Waddrdrvn2 = 0.0;
    circuit_param_t::cparam_Waddrdrvp2 = 0.0;

    circuit_param_t::cparam_Wdecdrivep_second = 0.0;
    circuit_param_t::cparam_Wdecdriven_second = 0.0;
    circuit_param_t::cparam_Wdecdrivep_first = 0.0;
    circuit_param_t::cparam_Wdecdriven_first = 0.0;
    circuit_param_t::cparam_Wdec3to8n = 0.0;
    circuit_param_t::cparam_Wdec3to8p = 0.0;
    circuit_param_t::cparam_WdecNORn  = 0.0;
    circuit_param_t::cparam_WdecNORp  = 0.0;
    circuit_param_t::cparam_Wdecinvn  = 0.0;
    circuit_param_t::cparam_Wdecinvp  = 0.0;
    circuit_param_t::cparam_WwlDrvn = 0.0;
    circuit_param_t::cparam_WwlDrvp = 0.0;



    circuit_param_t::cparam_Wbitpreequ = 0.0;

    circuit_param_t::cparam_Wiso = 0.0;
    circuit_param_t::cparam_Wpch = 0.0;
    circuit_param_t::cparam_Wiso = 0.0;
    circuit_param_t::cparam_WsenseEn = 0.0;
    circuit_param_t::cparam_WsenseN = 0.0;
    circuit_param_t::cparam_WsenseP = 0.0;
    circuit_param_t::cparam_WoBufN = 0.0;
    circuit_param_t::cparam_WoBufP = 0.0;
    circuit_param_t::cparam_WsPch = 0.0;

    circuit_param_t::cparam_WpchDrvp = 0.0; circuit_param_t::cparam_WpchDrvn = 0.0;
    circuit_param_t::cparam_WisoDrvp = 0.0; circuit_param_t::cparam_WisoDrvn = 0.0;
    circuit_param_t::cparam_WspchDrvp = 0.0; circuit_param_t::cparam_WspchDrvn = 0.0;
    circuit_param_t::cparam_WsenseEnDrvp = 0.0; circuit_param_t::cparam_WsenseEnDrvn = 0.0;

    circuit_param_t::cparam_WwrtMuxSelDrvn = 0.0;
    circuit_param_t::cparam_WwrtMuxSelDrvp = 0.0;

    circuit_param_t::cparam_WmuxdrvNANDn    = 0.0;
    circuit_param_t::cparam_WmuxdrvNANDp    = 0.0;
    circuit_param_t::cparam_WmuxdrvNORn	= 0.0;
    circuit_param_t::cparam_WmuxdrvNORp	= 0.0;
    circuit_param_t::cparam_Wmuxdrv3n	= 0.0;
    circuit_param_t::cparam_Wmuxdrv3p	= 0.0;
    circuit_param_t::cparam_Woutdrvseln	= 0.0;
    circuit_param_t::cparam_Woutdrvselp	= 0.0;

    circuit_param_t::cparam_Woutdrvnandn = 0.0;
    circuit_param_t::cparam_Woutdrvnandp = 0.0;
    circuit_param_t::cparam_Woutdrvnorn	= 0.0;
    circuit_param_t::cparam_Woutdrvnorp	= 0.0;
    circuit_param_t::cparam_Woutdrivern	= 0.0;
    circuit_param_t::cparam_Woutdriverp	= 0.0;
}

void cacti_time_t::compute_device_widths(int C, int B, int A, int fullyassoc, int Ndwl, int Ndbl, double Nspd)
{
    int rows, cols, numstack, l_predec_nor_v, l_predec_nor_h;
    double desiredrisetime, Rpdrive, Cline, Cload;
    double effWdecNORn, effWdecNORp, effWdec3to8n, effWdec3to8p, wire_res, wire_cap;
    int l_outdrv_v, l_outdrv_h;
    //int rows_fa_subarray,cols_fa_subarray, tagbits;
    int tagbits;
    int horizontal_edge = 0;
    int nr_subarrays_left = 0, v_or_h = 0;
    int horizontal_step = 0, vertical_step = 0;
    int h_inv_predecode = 0, v_inv_predecode = 0;

    double previous_ndriveW = 0, previous_pdriveW = 0, current_ndriveW = 0, current_pdriveW = 0;
    int i;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //rows = C/(CHUNKSIZE*B*A*Ndbl*Nspd);
    rows = (int) (C / (CHUNKSIZE * B * A * Ndbl * Nspd) + EPSILON);
    //cols = CHUNKSIZE*B*A*Nspd/Ndwl;
    cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);

    // Wordline capacitance to determine the wordline driver size
    /* Use a first-order approx */
    desiredrisetime = circuit_param_t::cparam_krise * log((double)(cols)) / 2.0;
    /*dt: I'm changing this back to what's in CACTI (as opposed to eCacti), i.e. counting the short poly connection to the pass transistors*/
    Cline = (bct->gatecappass(circuit_param_t::cparam_Wmemcella,
                              (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             bct->gatecappass(circuit_param_t::cparam_Wmemcella,
                              (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             circuit_param_t::cparam_Cwordmetal) * cols;
    Rpdrive = desiredrisetime / (Cline * log(VSINV) * -1.0);
    circuit_param_t::cparam_WwlDrvp = bct->restowidth(Rpdrive, PCH);
    if (circuit_param_t::cparam_WwlDrvp > circuit_param_t::cparam_Wworddrivemax) {
        circuit_param_t::cparam_WwlDrvp = circuit_param_t::cparam_Wworddrivemax;
    }

    /* Now that we have a reasonable psize, do the rest as before */
    /* If we keep the ratio the same as the tag wordline driver,
       the threshold voltage will be close to VSINV */

    /* assuming that nsize is half the psize */
    circuit_param_t::cparam_WwlDrvn = circuit_param_t::cparam_WwlDrvp / 2;

    // Size of wordline
    // Sizing ratio for optimal delay is 3-4.
    circuit_param_t::cparam_Wdecinvn = (circuit_param_t::cparam_WwlDrvp + circuit_param_t::cparam_WwlDrvn) * SizingRatio * 1 / 3;
    circuit_param_t::cparam_Wdecinvp = (circuit_param_t::cparam_WwlDrvp + circuit_param_t::cparam_WwlDrvn) * SizingRatio * 2 / 3;

    // determine size of nor and nand gates in the decoder

    // width of NOR driving decInv -
    // effective width (NORp + NORn = circuit_param_t::cparam_Cout/SizingRatio( FANOUT))
    // circuit_param_t::cparam_Cout = circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp; SizingRatio = 3;
    // nsize = effWidth/3; psize = 2*effWidth/3;

    numstack =
        (int)ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ndbl * Nspd))));
    if (numstack == 0) numstack = 1;

    if (numstack > 5) numstack = 5;

    effWdecNORn = (circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp) * SizingRatio / 3;
    effWdecNORp = 2 * (circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp) * SizingRatio / 3;
    circuit_param_t::cparam_WdecNORn = effWdecNORn;
    circuit_param_t::cparam_WdecNORp = effWdecNORp * numstack;

    /* second stage: driving a bunch of nor gates with a nand */

    /*dt: The *8 is there because above we mysteriously divide the capacity in BYTES by the number of BITS per wordline */
    l_predec_nor_v = rows * CHUNKSIZE;
    /*dt: If we follow the original drawings from the TR's, then there is almost no horizontal wires, only the poly for contacting
    the nor gates. The poly part we don't model right now */
    l_predec_nor_h = 0;

    //v4.1: Scaling the poly length to the input tech node.
    //Cline = bct->gatecap(circuit_param_t::cparam_WdecNORn+circuit_param_t::cparam_WdecNORp,((numstack*40)+20.0))*rows/8 +
    //circuit_param_t::cparam_GlobalCbitmetal*(l_predec_nor_v)+circuit_param_t::cparam_GlobalCwordmetal*(l_predec_nor_h);

    Cline = bct->gatecap(circuit_param_t::cparam_WdecNORn + circuit_param_t::cparam_WdecNORp, ((numstack * 40 / circuit_param_t::cparam_FUDGEFACTOR) + 20.0 / circuit_param_t::cparam_FUDGEFACTOR)) * rows / 8 +
            circuit_param_t::cparam_GlobalCbitmetal * (l_predec_nor_v) + circuit_param_t::cparam_GlobalCwordmetal * (l_predec_nor_h);

    Cload = Cline / bct->gatecap(1.0, 0.0);

    effWdec3to8n = Cload * SizingRatio / 3;
    effWdec3to8p = 2 * Cload * SizingRatio / 3;

    circuit_param_t::cparam_Wdec3to8n = effWdec3to8n * 3; // nand3 gate
    circuit_param_t::cparam_Wdec3to8p = effWdec3to8p;

    // size of address drivers before decoders
    /* First stage: driving the decoders */
    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //horizontal_edge = CHUNKSIZE*B*A*Nspd;
    horizontal_edge = (int) (CHUNKSIZE * B * A * Nspd + EPSILON);

    previous_ndriveW = circuit_param_t::cparam_Wdec3to8n;
    previous_pdriveW = circuit_param_t::cparam_Wdec3to8p;

    if (Ndwl*Ndbl == 1 ) {
        wire_cap = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        wire_res = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;
        circuit_param_t::cparam_Cdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        circuit_param_t::cparam_Rdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;

        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        circuit_param_t::cparam_nr_dectreesegments = 0;
    }
    else if (Ndwl*Ndbl == 2 || Ndwl*Ndbl == 4) {
        wire_cap = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        wire_res = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;
        circuit_param_t::cparam_Cdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        circuit_param_t::cparam_Rdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;

        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        circuit_param_t::cparam_nr_dectreesegments = 0;
    }
    else {
        /*dt: For the critical path  in an H-Tree, the metal */

        nr_subarrays_left = Ndwl * Ndbl;
        /*all the wires go to quads of subarrays where they get predecoded*/
        nr_subarrays_left /= 4;
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //horizontal_step = CHUNKSIZE*B*A*Nspd/Ndwl;
        horizontal_step = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);
        //vertical_step = C/(B*A*Ndbl*Nspd);
        vertical_step = (int) (C / (B * A * Ndbl * Nspd) + EPSILON);
        h_inv_predecode = horizontal_step;

        circuit_param_t::cparam_Cdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        circuit_param_t::cparam_Rdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;
        circuit_param_t::cparam_WdecdrivetreeN[0] = current_ndriveW;
        circuit_param_t::cparam_nr_dectreesegments = 1;

        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            previous_ndriveW = current_ndriveW;
            previous_pdriveW = current_pdriveW;
            circuit_param_t::cparam_nr_dectreesegments++;
            if (v_or_h) {
                v_inv_predecode += vertical_step;
                circuit_param_t::cparam_Cdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1] = circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                circuit_param_t::cparam_Rdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRbitmetal * vertical_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                v_or_h = 0;
                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                h_inv_predecode += horizontal_step;
                circuit_param_t::cparam_Cdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                circuit_param_t::cparam_Rdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                v_or_h = 1;
                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }
            Cload = Cline / bct->gatecap(1.0, 0.0);

            current_ndriveW = Cload * SizingRatio / 3;
            current_pdriveW = 2 * Cload * SizingRatio / 3;

            circuit_param_t::cparam_WdecdrivetreeN[circuit_param_t::cparam_nr_dectreesegments-1] = current_ndriveW;
        }

        if (circuit_param_t::cparam_nr_dectreesegments >= 10) {
            printf("Too many segments in the data decoder H-tree. Overflowing the preallocated array!");
            exit(1);
        }
        wire_cap = circuit_param_t::cparam_GlobalCbitmetal * v_inv_predecode + circuit_param_t::cparam_GlobalCwordmetal * h_inv_predecode;
        wire_res = 0.5 * (circuit_param_t::cparam_GlobalRbitmetal * v_inv_predecode + circuit_param_t::cparam_GlobalRwordmetal * h_inv_predecode);


    }

    circuit_param_t::cparam_Wdecdriven_second = current_ndriveW;
    circuit_param_t::cparam_Wdecdrivep_second = current_pdriveW;


    // Size of second driver

    circuit_param_t::cparam_Wdecdriven_first = (circuit_param_t::cparam_Wdecdriven_second + circuit_param_t::cparam_Wdecdrivep_second) * SizingRatio / 3;
    circuit_param_t::cparam_Wdecdrivep_first = 2 * (circuit_param_t::cparam_Wdecdriven_second + circuit_param_t::cparam_Wdecdrivep_second) * SizingRatio / 3;

    // these are the widths of the devices of dataoutput devices
    // will be used in the data_senseamplifier_data and dataoutput_data functions

    l_outdrv_v = 0;
    l_outdrv_h = 0;

    if (!fullyassoc) {

        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //rows =  (C/(B*A*Ndbl*Nspd));
        //cols = (CHUNKSIZE*B*A*Nspd/Ndwl);
        rows =  (int) (C / (B * A * Ndbl * Nspd) + EPSILON);
        cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);
    }
    else {
        rows = (C / (B * Ndbl));
        if (!force_tag) {
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS-bct->logtwo((double)B);
            tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - (int)(bct->logtwo((double)B) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols = (CHUNKSIZE * B) + tagbits;
    }

    /* calculate some layout info */


    if (Ndwl*Ndbl == 1) {
        l_outdrv_v = 0;
        l_outdrv_h = cols;

        Coutdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * cols;
        Routdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * cols;

        Cline = bct->gatecap(circuit_param_t::cparam_Wsenseextdrv1n + circuit_param_t::cparam_Wsenseextdrv1p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * cols;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        nr_outdrvtreesegments = 0;
    }
    else if (Ndwl*Ndbl == 2) {
        l_outdrv_v = 0;
        l_outdrv_h = 2 * cols;

        Coutdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * 2 * cols;
        Routdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * 2 * cols;

        Cline = bct->gatecap(circuit_param_t::cparam_Wsenseextdrv1n + circuit_param_t::cparam_Wsenseextdrv1p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * 2 * cols;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        nr_outdrvtreesegments = 0;
    }
    else if (Ndwl*Ndbl > 2) {
        nr_subarrays_left = Ndwl * Ndbl;
        nr_subarrays_left /= 2;
        /*dt: assuming the sense amps are in the middle of each subarray */
        horizontal_step = cols / 2;
        vertical_step = rows / 2;
        l_outdrv_h = horizontal_step;

        Coutdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        Routdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
        nr_outdrvtreesegments = 1;

        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            nr_outdrvtreesegments++;
            if (v_or_h) {
                l_outdrv_v += vertical_step;

                Coutdrvtreesegments[nr_outdrvtreesegments-1] = circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                Routdrvtreesegments[nr_outdrvtreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRbitmetal * vertical_step;

                v_or_h = 0;
                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                l_outdrv_h += horizontal_step;

                Coutdrvtreesegments[nr_outdrvtreesegments-1] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                Routdrvtreesegments[nr_outdrvtreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;

                v_or_h = 1;
                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }
        }

        /*dt: Now that we have all the H-tree segments for the output tree,
        we can walk it in reverse and calc the gate widths*/

        previous_ndriveW = circuit_param_t::cparam_Wsenseextdrv1n;
        previous_pdriveW = circuit_param_t::cparam_Wsenseextdrv1p;
        for (i = nr_outdrvtreesegments - 1;i > 0;i--) {
            Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + Coutdrvtreesegments[i];
            Cload = Cline / bct->gatecap(1.0, 0.0);

            current_ndriveW = Cload * SizingRatio / 3;
            current_pdriveW = 2 * Cload * SizingRatio / 3;

            WoutdrvtreeN[i] = current_ndriveW;

            previous_ndriveW = current_ndriveW;
            previous_pdriveW = current_pdriveW;
        }
    }

    if (nr_outdrvtreesegments >= 20) {
        printf("Too many segments in the output H-tree. Overflowing the preallocated array!");
        exit(1);
    }
    // typical width of gate considered for the estimating val of draincap.
    Cload = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + Coutdrvtreesegments[0] +
            (bct->draincap(5.0 / circuit_param_t::cparam_FUDGEFACTOR, NCH, 1) + bct->draincap(5.0 / circuit_param_t::cparam_FUDGEFACTOR, PCH, 1)) * A * circuit_param_t::cparam_muxover;

    circuit_param_t::cparam_Woutdrivern = 	(Cload / bct->gatecap(1.0, 0.0)) * SizingRatio / 3;
    circuit_param_t::cparam_Woutdriverp = 	(Cload / bct->gatecap(1.0, 0.0)) * SizingRatio * 2 / 3;

    // eff load for nor gate = bct->gatecap(drv_p);
    // factor of 2 is needed to account for series nmos transistors in nor2
    circuit_param_t::cparam_Woutdrvnorp = 2 * circuit_param_t::cparam_Woutdriverp * SizingRatio * 2 / 3;
    circuit_param_t::cparam_Woutdrvnorn = circuit_param_t::cparam_Woutdriverp * SizingRatio / 3;

    // factor of 2 is needed to account for series nmos transistors in nand2
    circuit_param_t::cparam_Woutdrvnandp = circuit_param_t::cparam_Woutdrivern * SizingRatio * 2 / 3;
    circuit_param_t::cparam_Woutdrvnandn = 2 * circuit_param_t::cparam_Woutdrivern * SizingRatio / 3;

    circuit_param_t::cparam_Woutdrvselp = (circuit_param_t::cparam_Woutdrvnandp + circuit_param_t::cparam_Woutdrvnandn) * SizingRatio * 2 / 3;
    circuit_param_t::cparam_Woutdrvseln = (circuit_param_t::cparam_Woutdrvnandp + circuit_param_t::cparam_Woutdrvnandn) * SizingRatio / 3;

}


void cacti_time_t::compute_tag_device_widths(int C, int B, int A, int Ntspd, int Ntwl, int Ntbl, double NSubbanks)
{

    int rows, cols, tagbits, numstack, l_predec_nor_v, l_predec_nor_h;
    double Cline, Cload, Rpdrive, desiredrisetime;
    double effWtdecNORn, effWtdecNORp, effWtdec3to8n, effWtdec3to8p, wire_res, wire_cap;
    int horizontal_edge = 0;
    int nr_subarrays_left = 0, v_or_h = 0;
    int horizontal_step = 0, vertical_step = 0;
    int h_inv_predecode = 0, v_inv_predecode = 0;

    double previous_ndriveW = 0, previous_pdriveW = 0, current_ndriveW = 0, current_pdriveW = 0;

    rows = C / (CHUNKSIZE * B * A * Ntbl * Ntspd);
    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS-bct->logtwo((double)C)+bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - bct->logtwo(NSubbanks) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    cols = tagbits * A * Ntspd / Ntwl;

    // capacitive load on the wordline - C_int + C_memCellLoad * NCells
    Cline = (bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) + circuit_param_t::cparam_TagCwordmetal) * cols;

    /*dt: changing the calculations for the tag wordline to be the same as for the data wordline*/
    /* Use a first-order approx */
    desiredrisetime = circuit_param_t::cparam_krise * log((double)(cols)) / 2.0;

    Rpdrive = desiredrisetime / (Cline * log(VSINV) * -1.0);
    circuit_param_t::cparam_WtwlDrvp = bct->restowidth(Rpdrive, PCH);
    if (circuit_param_t::cparam_WtwlDrvp > circuit_param_t::cparam_Wworddrivemax) {
        circuit_param_t::cparam_WtwlDrvp = circuit_param_t::cparam_Wworddrivemax;
    }

    circuit_param_t::cparam_WtwlDrvn = circuit_param_t::cparam_WtwlDrvp / 2;

    circuit_param_t::cparam_Wtdecinvn = (circuit_param_t::cparam_WtwlDrvn + circuit_param_t::cparam_WtwlDrvp) * SizingRatio * 1 / 3;
    circuit_param_t::cparam_Wtdecinvp = (circuit_param_t::cparam_WtwlDrvn + circuit_param_t::cparam_WtwlDrvp) * SizingRatio * 2 / 3;

    // determine widths of nand, nor gates in the tag decoder
    // width of NOR driving decInv -
    // effective width (NORp + NORn = circuit_param_t::cparam_Cout/SizingRatio( FANOUT))
    // circuit_param_t::cparam_Cout = circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp; SizingRatio = 3;
    // nsize = effWidth/3; psize = 2*effWidth/3;
    numstack =
        (int)ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ntbl * Ntspd))));
    if (numstack == 0) numstack = 1;
    if (numstack > 5) numstack = 5;

    effWtdecNORn = (circuit_param_t::cparam_Wtdecinvn + circuit_param_t::cparam_Wtdecinvp) * SizingRatio / 3;
    effWtdecNORp = 2 * (circuit_param_t::cparam_Wtdecinvn + circuit_param_t::cparam_Wtdecinvp) * SizingRatio / 3;
    circuit_param_t::cparam_WtdecNORn = effWtdecNORn;
    circuit_param_t::cparam_WtdecNORp = effWtdecNORp * numstack;

    /*dt: The *8 is there because above we mysteriously divide the capacity in BYTES by the number of BITS per wordline */
    l_predec_nor_v = rows * 8;
    /*dt: If we follow the original drawings from the TR's, then there is almost no horizontal wires, only the poly for contacting
    the nor gates. The poly part we don't model right now */
    l_predec_nor_h = 0;

    // find width of the nand gates in the 3-8 decoders
    Cline = bct->gatecap(circuit_param_t::cparam_WtdecNORn + circuit_param_t::cparam_WtdecNORp, ((numstack * 40) / circuit_param_t::cparam_FUDGEFACTOR + 20.0 / circuit_param_t::cparam_FUDGEFACTOR)) * rows / 8 +
            circuit_param_t::cparam_GlobalCbitmetal * (l_predec_nor_v) + circuit_param_t::cparam_GlobalCwordmetal * (l_predec_nor_h);

    Cload = Cline / bct->gatecap(1.0, 0.0);

    effWtdec3to8n = Cload * SizingRatio / 3;
    effWtdec3to8p = 2 * Cload * SizingRatio / 3;

    circuit_param_t::cparam_Wtdec3to8n = effWtdec3to8n * 3; // nand3 gate
    circuit_param_t::cparam_Wtdec3to8p = effWtdec3to8p;

    horizontal_edge = cols * Ntwl;

    previous_ndriveW = circuit_param_t::cparam_Wtdec3to8n;
    previous_pdriveW = circuit_param_t::cparam_Wtdec3to8p;

    if (Ntwl*Ntbl == 1 ) {
        wire_cap = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        wire_res = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;

        circuit_param_t::cparam_Ctdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        circuit_param_t::cparam_Rtdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;

        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        circuit_param_t::cparam_nr_tdectreesegments = 0;
    }
    else if (Ntwl*Ntbl == 2 || Ntwl*Ntbl == 4) {
        wire_cap = circuit_param_t::cparam_GlobalCwordmetal * 0.5 * horizontal_edge;
        wire_res = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * 0.5 * horizontal_edge;

        circuit_param_t::cparam_Ctdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        circuit_param_t::cparam_Rtdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_edge;

        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_edge;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        circuit_param_t::cparam_nr_tdectreesegments = 0;
    }
    else {
        nr_subarrays_left = Ntwl * Ntbl;
        nr_subarrays_left /= 4;
        horizontal_step = cols;
        vertical_step = C / (B * A * Ntbl * Ntspd);
        h_inv_predecode = horizontal_step;

        circuit_param_t::cparam_Ctdectreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        circuit_param_t::cparam_Rtdectreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
        Cline = 4 * bct->gatecap(previous_ndriveW + previous_pdriveW, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;
        circuit_param_t::cparam_WtdecdrivetreeN[0] = current_ndriveW;
        circuit_param_t::cparam_nr_tdectreesegments = 1;

        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            previous_ndriveW = current_ndriveW;
            previous_pdriveW = current_pdriveW;
            circuit_param_t::cparam_nr_tdectreesegments++;
            if (v_or_h) {
                v_inv_predecode += vertical_step;
                circuit_param_t::cparam_Ctdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1] = circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                circuit_param_t::cparam_Rtdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRbitmetal * vertical_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                v_or_h = 0;
                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                h_inv_predecode += horizontal_step;
                circuit_param_t::cparam_Ctdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                circuit_param_t::cparam_Rtdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                v_or_h = 1;
                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }
            Cload = Cline / bct->gatecap(1.0, 0.0);

            current_ndriveW = Cload * SizingRatio / 3;
            current_pdriveW = 2 * Cload * SizingRatio / 3;

            circuit_param_t::cparam_WtdecdrivetreeN[circuit_param_t::cparam_nr_tdectreesegments-1] = current_ndriveW;
        }

        if (circuit_param_t::cparam_nr_tdectreesegments >= 10) {
            printf("Too many segments in the tag decoder H-tree. Overflowing the preallocated array!");
            exit(1);
        }

        wire_cap = circuit_param_t::cparam_GlobalCbitmetal * v_inv_predecode + circuit_param_t::cparam_GlobalCwordmetal * h_inv_predecode;
        wire_res = 0.5 * (circuit_param_t::cparam_GlobalRbitmetal * v_inv_predecode + circuit_param_t::cparam_GlobalRwordmetal * h_inv_predecode);
    }

    Cline = 4 * bct->gatecap(circuit_param_t::cparam_Wtdec3to8n + circuit_param_t::cparam_Wtdec3to8p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + wire_cap;
    Cload = Cline / bct->gatecap(1.0, 0.0);

    circuit_param_t::cparam_Wtdecdriven_second = current_ndriveW;
    circuit_param_t::cparam_Wtdecdrivep_second = current_pdriveW;

    // Size of second driver

    circuit_param_t::cparam_Wtdecdriven_first = (circuit_param_t::cparam_Wtdecdriven_second + circuit_param_t::cparam_Wtdecdrivep_second) * SizingRatio / 3;
    circuit_param_t::cparam_Wtdecdrivep_first = 2 * (circuit_param_t::cparam_Wtdecdriven_second + circuit_param_t::cparam_Wtdecdrivep_second) * SizingRatio / 3;

}


double cacti_time_t::cmos_ileakage(double nWidth, double pWidth,
                                   double nVthresh_dual, double nVthreshold, double pVthresh_dual, double pVthreshold) {
    double leakage = 0.0;


    static int valid_cache = 0;
    static double cached_nmos_thresh = 0;
    static double cached_pmos_thresh = 0;

    static double norm_nmos_leakage = 0;
    static double norm_pmos_leakage = 0;

    if (leakage_obj->leakage_params()) {

        if (circuit_param_t::cparam_dualVt == TRUE) {

            if ((cached_nmos_thresh == nVthresh_dual) && (cached_pmos_thresh == pVthresh_dual) && valid_cache) {
                leakage = nWidth * norm_nmos_leakage + pWidth * norm_pmos_leakage;
            }
            else {
                leakage = leakage_obj->simplified_cmos_leakage(nWidth * circuit_param_t::cparam_inv_Leff, 
                    pWidth * circuit_param_t::cparam_inv_Leff, nVthresh_dual,
                    pVthresh_dual, &norm_nmos_leakage, &norm_pmos_leakage);
                cached_nmos_thresh = nVthresh_dual;
                cached_pmos_thresh = pVthresh_dual;
                norm_nmos_leakage = circuit_param_t::cparam_inv_Leff * norm_nmos_leakage;
                norm_pmos_leakage = circuit_param_t::cparam_inv_Leff * norm_pmos_leakage;
                valid_cache = 1;
            }
        }
        else {

            if ((cached_nmos_thresh == nVthreshold) && (cached_pmos_thresh == pVthreshold) && valid_cache) {
                leakage = nWidth * norm_nmos_leakage + pWidth * norm_pmos_leakage;
            }
            else {
                leakage = leakage_obj->simplified_cmos_leakage(nWidth * circuit_param_t::cparam_inv_Leff, pWidth * circuit_param_t::cparam_inv_Leff, nVthreshold, pVthreshold, &norm_nmos_leakage, &norm_pmos_leakage);
                cached_nmos_thresh = nVthreshold;
                cached_pmos_thresh = pVthreshold;
                norm_nmos_leakage = circuit_param_t::cparam_inv_Leff * norm_nmos_leakage;
                norm_pmos_leakage = circuit_param_t::cparam_inv_Leff * norm_pmos_leakage;
                valid_cache = 1;
            }
        }
    }

    return leakage;
}
void cacti_time_t::reset_powerDef(powerDef *power) {
    power->readOp.dynamic = 0.0;
    power->readOp.leakage = 0.0;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = 0.0;
}
void cacti_time_t::mult_powerDef(powerDef *power, int val) {
    power->readOp.dynamic *= val;
    power->readOp.leakage *= val;

    power->writeOp.dynamic *= val;
    power->writeOp.leakage *= val;
}
void cacti_time_t::mac_powerDef(powerDef *sum, powerDef *mult, int val) {
    sum->readOp.dynamic += mult->readOp.dynamic * val;
    sum->readOp.leakage += mult->readOp.leakage * val;

    sum->writeOp.dynamic += mult->writeOp.dynamic * val;
    sum->writeOp.leakage += mult->writeOp.leakage * val;
}
void cacti_time_t::copy_powerDef(powerDef *dest, powerDef source) {
    dest->readOp.dynamic = source.readOp.dynamic;
    dest->readOp.leakage = source.readOp.leakage;

    dest->writeOp.dynamic = source.writeOp.dynamic;
    dest->writeOp.leakage = source.writeOp.leakage;
}
void cacti_time_t::copy_and_div_powerDef(powerDef *dest, powerDef source, double val) {

    // only dynamic power needs to be scaled.
    dest->readOp.dynamic = source.readOp.dynamic / val;
    dest->readOp.leakage = source.readOp.leakage;

    dest->writeOp.dynamic = source.writeOp.dynamic / val;
    dest->writeOp.leakage = source.writeOp.leakage;
}

void cacti_time_t::add_powerDef(powerDef *sum, powerDef a, powerDef b) {

    sum->readOp.dynamic = a.readOp.dynamic + b.readOp.dynamic;
    sum->readOp.leakage = a.readOp.leakage + b.readOp.leakage;

    sum->writeOp.dynamic = a.writeOp.dynamic + b.writeOp.dynamic;
    sum->writeOp.leakage = a.writeOp.leakage + b.writeOp.leakage;
}

double cacti_time_t::objective_function(double delay_weight, double area_weight, double power_weight,
                                        double delay, double area, double power)
{
    return
        (double)(area_weight*area + delay_weight*delay + power_weight*power);
}



/*======================================================================*/



/*
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

void cacti_time_t::subbank_routing_length (int C, int B, int A,
        char fullyassoc,
        int Ndbl, double Nspd, int Ndwl, int Ntbl, int Ntwl, int Ntspd,
        double NSubbanks,
        double *subbank_v, double *subbank_h)
{
    double htree;
    int htree_int, tagbits;
    int cols_data_subarray, rows_data_subarray, cols_tag_subarray,
    rows_tag_subarray;
    double inter_v, inter_h, sub_h, sub_v;
    int inter_subbanks;
    int cols_fa_subarray, rows_fa_subarray;

    if (!fullyassoc)
    {

        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //cols_data_subarray = (8 * B * A * Nspd / Ndwl);
        //rows_data_subarray = (C / (B * A * Ndbl * Nspd));
        cols_data_subarray = (int) ((8 * B * A * Nspd / Ndwl) + EPSILON);
        rows_data_subarray = (int) (C / (B * A * Ndbl * Nspd) + EPSILON);

        if (Ndwl * Ndbl == 1)
        {
            sub_v = rows_data_subarray;
            sub_h = cols_data_subarray;
        }
        else if (Ndwl * Ndbl == 2)
        {
            sub_v = rows_data_subarray;
            sub_h = 2 * cols_data_subarray;
        }
        else if (Ndwl * Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndwl * Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl)) * rows_data_subarray;
                sub_h = sqrt ((double) (Ndwl * Ndbl)) * cols_data_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl / 2)) * rows_data_subarray;
                sub_h = 2 * sqrt ((double) (Ndwl * Ndbl / 2)) * cols_data_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        rows_tag_subarray = C / (B * A * Ntbl * Ntspd);
        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
            //bct->logtwo ((double) A) - (int) (bct->logtwo (NSubbanks));
            tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
                             bct->logtwo ((double) A) - (bct->logtwo (NSubbanks)) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }

        cols_tag_subarray = tagbits * A * Ntspd / Ntwl;

        if (Ntwl * Ntbl == 1)
        {
            sub_v = rows_tag_subarray;
            sub_h = cols_tag_subarray;
        }
        if (Ntwl * Ntbl == 2)
        {
            sub_v = rows_tag_subarray;
            sub_h = 2 * cols_tag_subarray;
        }

        if (Ntwl * Ntbl > 2)
        {
            htree = bct->logtwo ((double) (Ntwl * Ntbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl)) * rows_tag_subarray;
                sub_h = sqrt ((double) (Ntwl * Ntbl)) * cols_tag_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl / 2)) * rows_tag_subarray;
                sub_h = 2 * sqrt ((double) (Ntwl * Ntbl / 2)) * cols_tag_subarray;
            }
        }


        inter_v = MAX (sub_v, inter_v);
        inter_h += sub_h;

        sub_v = 0;
        sub_h = 0;

        if (NSubbanks == 1.0 || NSubbanks == 2.0)
        {
            sub_h = 0;
            sub_v = 0;
        }
        if (NSubbanks == 4.0)
        {
            sub_h = 0;
            sub_v = inter_v;
        }

        inter_subbanks = (int)NSubbanks;

        while ((inter_subbanks > 2) && (NSubbanks > 4))
        {

            sub_v += inter_v;
            sub_h += inter_h;

            inter_v = 2 * inter_v;
            inter_h = 2 * inter_h;
            inter_subbanks = inter_subbanks / 4;

            if (inter_subbanks == 4.0)
            {
                inter_h = 0;
            }

        }
        *subbank_v = sub_v;
        *subbank_h = sub_h;
    }
    else
    {
        rows_fa_subarray = (C / (B * Ndbl));
        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) B);
            tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - (bct->logtwo ((double) B) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_fa_subarray = (8 * B) + tagbits;

        if (Ndbl == 1)
        {
            sub_v = rows_fa_subarray;
            sub_h = cols_fa_subarray;
        }
        if (Ndbl == 2)
        {
            sub_v = rows_fa_subarray;
            sub_h = 2 * cols_fa_subarray;
        }

        if (Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) Ndbl) * rows_fa_subarray;
                sub_h = sqrt ((double) Ndbl) * cols_fa_subarray;
            }
            else
            {
                sub_v = sqrt ((double)(Ndbl / 2)) * rows_fa_subarray;
                sub_h = 2 * sqrt ((double)(Ndbl / 2)) * cols_fa_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        sub_v = 0;
        sub_h = 0;

        if (NSubbanks == 1.0 || NSubbanks == 2.0)
        {
            sub_h = 0;
            sub_v = 0;
        }
        if (NSubbanks == 4.0)
        {
            sub_h = 0;
            sub_v = inter_v;
        }

        inter_subbanks = (int)NSubbanks;

        while ((inter_subbanks > 2) && (NSubbanks > 4))
        {

            sub_v += inter_v;
            sub_h += inter_h;

            inter_v = 2 * inter_v;
            inter_h = 2 * inter_h;
            inter_subbanks = inter_subbanks / 4;

            if (inter_subbanks == 4.0)
            {
                inter_h = 0;
            }

        }
        *subbank_v = sub_v;
        *subbank_h = sub_h;
    }
}

void cacti_time_t::subbank_dim (int C, int B, int A,
                                char fullyassoc,
                                int Ndbl, int Ndwl, double Nspd, int Ntbl, int Ntwl, int Ntspd,
                                double NSubbanks,
                                double *subbank_h, double *subbank_v)
{
    double htree;
    int htree_int, tagbits;
    int cols_data_subarray, rows_data_subarray, cols_tag_subarray,
    rows_tag_subarray;
    double sub_h, sub_v, inter_v, inter_h;
    int cols_fa_subarray, rows_fa_subarray;

    if (!fullyassoc)
    {

        /* calculation of subbank dimensions */
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //cols_data_subarray = (8 * B * A * Nspd / Ndwl);
        //rows_data_subarray = (C / (B * A * Ndbl * Nspd));
        cols_data_subarray = (int)(8 * B * A * Nspd / Ndwl + EPSILON);
        rows_data_subarray = (int) (C / (B * A * Ndbl * Nspd) + EPSILON);

        if (Ndwl * Ndbl == 1)
        {
            sub_v = rows_data_subarray;
            sub_h = cols_data_subarray;
        }
        if (Ndwl * Ndbl == 2)
        {
            sub_v = rows_data_subarray;
            sub_h = 2 * cols_data_subarray;
        }
        if (Ndwl * Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndwl * Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl)) * rows_data_subarray;
                sub_h = sqrt ((double) (Ndwl * Ndbl)) * cols_data_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl / 2)) * rows_data_subarray;
                sub_h = 2 * sqrt ((double) (Ndwl * Ndbl / 2)) * cols_data_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        rows_tag_subarray = C / (B * A * Ntbl * Ntspd);

        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
            //bct->logtwo ((double) A) - (int) (bct->logtwo (NSubbanks));
            tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
                             bct->logtwo ((double) A) - (bct->logtwo (NSubbanks)) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_tag_subarray = tagbits * A * Ntspd / Ntwl;

        if (Ntwl * Ntbl == 1)
        {
            sub_v = rows_tag_subarray;
            sub_h = cols_tag_subarray;
        }
        if (Ntwl * Ntbl == 2)
        {
            sub_v = rows_tag_subarray;
            sub_h = 2 * cols_tag_subarray;
        }

        if (Ntwl * Ntbl > 2)
        {
            htree = bct->logtwo ((double) (Ntwl * Ntbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl)) * rows_tag_subarray;
                sub_h = sqrt ((double) (Ntwl * Ntbl)) * cols_tag_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl / 2)) * rows_tag_subarray;
                sub_h = 2 * sqrt ((double) (Ntwl * Ntbl / 2)) * cols_tag_subarray;
            }
        }

        inter_v = MAX (sub_v, inter_v);
        inter_h += sub_h;

        *subbank_v = inter_v;
        *subbank_h = inter_h;
    }

    else
    {
        rows_fa_subarray = (C / (B * Ndbl));
        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) B);
            tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - (bct->logtwo ((double) B) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_fa_subarray = (8 * B) + tagbits;

        if (Ndbl == 1)
        {
            sub_v = rows_fa_subarray;
            sub_h = cols_fa_subarray;
        }
        if (Ndbl == 2)
        {
            sub_v = rows_fa_subarray;
            sub_h = 2 * cols_fa_subarray;
        }

        if (Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) Ndbl) * rows_fa_subarray;
                sub_h = sqrt ((double) Ndbl) * cols_fa_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ndbl / 2)) * rows_fa_subarray;
                sub_h = 2 * sqrt ((double) (Ndbl / 2)) * cols_fa_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        *subbank_v = inter_v;
        *subbank_h = inter_h;
    }
}


void cacti_time_t::subbanks_routing_power (char fullyassoc,
        int A,
        double NSubbanks,
        double *subbank_h, double *subbank_v, powerDef *power)
{
    double Ceq, Ceq_outdrv;
    int i, blocks, htree_int, subbank_mod;
    double htree, wire_cap, wire_cap_outdrv;
    double start_h, start_v, line_h, line_v;
    double dynPower = 0.0;

    //*power = 0.0;

    if (fullyassoc)
    {
        A = 1;
    }

    if (NSubbanks == 1.0 || NSubbanks == 2.0)
    {
        //*power = 0.0;
        power->writeOp.dynamic = 0.0;
        power->writeOp.leakage = 0.0;

        power->readOp.dynamic = 0.0;
        power->readOp.leakage = 0.0;
    }

    if (NSubbanks == 4.0)
    {
        /* calculation of address routing power */
        wire_cap = circuit_param_t::cparam_GlobalCbitmetal * (*subbank_v);
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
        dynPower += 2 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              wire_cap;
        dynPower += 2 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

        /* calculation of out driver power */
        wire_cap_outdrv = circuit_param_t::cparam_Cbitmetal * (*subbank_v);
        Ceq_outdrv =
            bct->draincap(circuit_param_t::cparam_Wsenseextdrv1p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv1n, NCH,
                    1) +
            bct->gatecap (circuit_param_t::cparam_Wsenseextdrv2n + circuit_param_t::cparam_Wsenseextdrv2p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        dynPower += 2 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
        Ceq_outdrv =
            bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH,
                    1) + wire_cap_outdrv;
        dynPower += 2 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;

    }

    if (NSubbanks == 8.0)
    {
        wire_cap = circuit_param_t::cparam_GlobalCbitmetal * (*subbank_v) + circuit_param_t::cparam_GlobalCwordmetal * (*subbank_h);
        /* buffer stage */
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
        dynPower += 6 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              wire_cap;
        dynPower += 4 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              (wire_cap - circuit_param_t::cparam_Cbitmetal * (*subbank_v));
        dynPower += 2 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

        /* calculation of out driver power */
        wire_cap_outdrv = circuit_param_t::cparam_Cbitmetal * (*subbank_v) + circuit_param_t::cparam_Cwordmetal * (*subbank_h);
        Ceq_outdrv =
            bct->draincap(circuit_param_t::cparam_Wsenseextdrv1p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv1n, NCH,
                    1) +
            bct->gatecap (circuit_param_t::cparam_Wsenseextdrv2n + circuit_param_t::cparam_Wsenseextdrv2p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        dynPower += 6 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
        Ceq_outdrv =
            bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH,
                    1) + wire_cap_outdrv;
        dynPower += 4 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
        Ceq_outdrv =
            bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH,
                    1) + (wire_cap_outdrv -
                          circuit_param_t::cparam_Cbitmetal *
                          (*subbank_v));
        dynPower += 2 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;

    }

    if (NSubbanks > 8.0)
    {
        blocks = (int) (NSubbanks / 8.0);
        htree = bct->logtwo ((double) (blocks));
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //htree_int = (int) htree;
        htree_int = (int) (htree + EPSILON);
        if (htree_int % 2 == 0)
        {
            subbank_mod = htree_int;
            start_h =
                (*subbank_h * (bct->powers (2, ((bct->logtwo(htree))) + 1) - 1));
            start_v = *subbank_v;
        }
        else
        {
            subbank_mod = htree_int - 1;
            start_h = (*subbank_h * (bct->powers (2, (htree_int + 1) / 2) - 1));
            start_v = *subbank_v;
        }

        if (subbank_mod == 0)
        {
            subbank_mod = 1;
        }

        line_v = start_v;
        line_h = start_h;

        for (i = 1; i <= blocks; i++)
        {
            wire_cap = line_v * circuit_param_t::cparam_GlobalCbitmetal + line_h * circuit_param_t::cparam_GlobalCwordmetal;

            Ceq =
                bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH,
                        1) +
                bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
            dynPower += 6 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
            Ceq =
                bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH,
                        1) + wire_cap;
            dynPower += 4 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
            Ceq =
                bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH,
                        1) + (wire_cap -
                              circuit_param_t::cparam_Cbitmetal *
                              (*subbank_v));
            dynPower += 2 * ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

            /* calculation of out driver power */
            wire_cap_outdrv = line_v * circuit_param_t::cparam_GlobalCbitmetal + line_h * circuit_param_t::cparam_GlobalCwordmetal;

            Ceq_outdrv =
                bct->draincap(circuit_param_t::cparam_Wsenseextdrv1p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv1n, NCH,
                        1) +
                bct->gatecap (circuit_param_t::cparam_Wsenseextdrv2n + circuit_param_t::cparam_Wsenseextdrv2p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
            dynPower +=
                6 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
            Ceq_outdrv =
                bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH,
                        1) +
                wire_cap_outdrv;
            dynPower +=
                4 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
            Ceq_outdrv =
                bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH,
                        1) +
                (wire_cap_outdrv - circuit_param_t::cparam_Cbitmetal * (*subbank_v));
            dynPower +=
                2 * Ceq_outdrv * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;

            if (i % subbank_mod == 0)
            {
                line_v += 2 * (*subbank_v);
            }
        }
    }

    power->writeOp.dynamic = dynPower;
    /*dt: still have to add leakage current*/
    power->readOp.dynamic = dynPower;
    /*dt: still have to add leakage current*/
}

double cacti_time_t::address_routing_delay (int C, int B, int A,
        char fullyassoc,
        int Ndwl, int Ndbl, double Nspd, int Ntwl, int Ntbl, int Ntspd,
        double *NSubbanks, double *outrisetime, powerDef *power)
{
    double Ceq, tf, nextinputtime, delay_stage1, delay_stage2;
    double addr_h, addr_v;
    double wire_cap, wire_res;
    double lkgCurrent = 0.0;
    double dynEnergy = 0.0;
    double Cline, Cload;
    //powerDef *thisPower;

    /* Calculate rise time.  Consider two inverters */

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wdecdriven, NCH, 1);
    nextinputtime = bct->horowitz (0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                    (VTHINV360x240);

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wdecdriven, NCH, 1);
    nextinputtime = bct->horowitz (nextinputtime, tf, VTHINV360x240, VTHINV360x240,
                                   RISE) / (1.0 - VTHINV360x240);
    addr_h = 0;
    addr_v = 0;
    subbank_routing_length (C, B, A, fullyassoc, Ndbl, Nspd, Ndwl, Ntbl, Ntwl,
                            Ntspd, *NSubbanks, &addr_v, &addr_h);
    wire_cap = circuit_param_t::cparam_GlobalCbitmetal * addr_v + addr_h * circuit_param_t::cparam_GlobalCwordmetal;
    wire_res = (circuit_param_t::cparam_GlobalRwordmetal * addr_h + circuit_param_t::cparam_GlobalRbitmetal * addr_v) / 2.0;

    /* buffer stage */
    /*dt: added gate width calc and leakage from eCACTI */
    Cline = *NSubbanks * ( bct->gatecap(circuit_param_t::cparam_Wdecdrivep_first + circuit_param_t::cparam_Wdecdriven_first, 0.0) + bct->gatecap(circuit_param_t::cparam_Wtdecdrivep_first + circuit_param_t::cparam_Wtdecdriven_first, 0.0) ) + wire_cap;
    Cload = Cline / bct->gatecap(1.0, 0.0);
    circuit_param_t::cparam_Waddrdrvn1 = Cload * SizingRatio / 3;
    circuit_param_t::cparam_Waddrdrvp1 = Cload * SizingRatio * 2 / 3;

    circuit_param_t::cparam_Waddrdrvn2 = (circuit_param_t::cparam_Waddrdrvn1 + circuit_param_t::cparam_Waddrdrvp1) * SizingRatio * 1 / 3 ;
    circuit_param_t::cparam_Waddrdrvp2 = (circuit_param_t::cparam_Waddrdrvn1 + circuit_param_t::cparam_Waddrdrvp1) * SizingRatio * 2 / 3 ;

    Ceq = bct->draincap(circuit_param_t::cparam_Waddrdrvp2, PCH, 1) + bct->draincap(circuit_param_t::cparam_Waddrdrvn2, NCH, 1) +
          bct->gatecap(circuit_param_t::cparam_Waddrdrvn1 + circuit_param_t::cparam_Waddrdrvp1, 0.0);
    tf = Ceq * bct->transreson(circuit_param_t::cparam_Waddrdrvn2, NCH, 1);
    delay_stage1 = bct->horowitz(nextinputtime, tf, VTHINV360x240, VTHINV360x240, FALL);
    nextinputtime = bct->horowitz(nextinputtime, tf, VTHINV360x240, VTHINV360x240, FALL) / (VTHINV360x240);

    dynEnergy = ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent += ADDRESS_BITS * 0.5 * cmos_ileakage(circuit_param_t::cparam_Waddrdrvn2, circuit_param_t::cparam_Waddrdrvp2, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    Ceq = bct->draincap(circuit_param_t::cparam_Waddrdrvp1, PCH, 1) + bct->draincap(circuit_param_t::cparam_Waddrdrvn1, NCH, 1) + wire_cap
          + *NSubbanks * (bct->gatecap(circuit_param_t::cparam_Wdecdrivep_first + circuit_param_t::cparam_Wdecdriven_first, 0.0) + bct->gatecap(circuit_param_t::cparam_Wtdecdrivep_first + circuit_param_t::cparam_Wtdecdriven_first, 0.0));
    tf = Ceq * (bct->transreson(circuit_param_t::cparam_Waddrdrvn1, NCH, 1) + wire_res);
    delay_stage2 = bct->horowitz(nextinputtime, tf, VTHINV360x240, VTHINV360x240, RISE);
    nextinputtime = bct->horowitz(nextinputtime, tf, VTHINV360x240, VTHINV360x240, RISE) / (1.0 - VTHINV360x240);

    dynEnergy += ADDRESS_BITS * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent += ADDRESS_BITS * 0.5 * cmos_ileakage(circuit_param_t::cparam_Waddrdrvn1, circuit_param_t::cparam_Waddrdrvp1, Vthn, Vt_bit_nmos_low, Vt_bit_pmos_low, Vthp);

    *outrisetime = nextinputtime;

    power->readOp.dynamic = dynEnergy;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    // power for write op same as read op for address routing
    power->writeOp.dynamic = dynEnergy;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    return(delay_stage1 + delay_stage2);

}



/* Decoder delay:  (see section 6.1 of tech report) */

/*dt: this is integrated from eCACTI. But we use want Energy per operation, not some measure of power, so no FREQ. */
double cacti_time_t::decoder_delay(int C, int B, int A, int Ndwl, int Ndbl, double Nspd, double NSubbanks,
                                   double *Tdecdrive, double *Tdecoder1, double *Tdecoder2, double inrisetime, double *outrisetime, int *nor_inputs, powerDef *power)
{
    int numstack, Nact;
    //double Ceq,Req,Rwire,tf,nextinputtime,vth,tstep;
    double Ceq, Req, Rwire, tf, nextinputtime, vth;
    int l_predec_nor_v, l_predec_nor_h, rows, cols;
    //double wire_cap, wire_res;
    double lkgCurrent = 0.0, dynPower = 0.0;
    int addr_bits_routed;
    int i;
    double this_delay;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //int addr_bits=bct->logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd)));
    //addr_bits_routed = bct->logtwo( (double)((double)C/(double)B));
    //int addr_bits=bct->logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd)));
    //addr_bits_routed = bct->logtwo( (double)((double)C/(double)B));

//	int addr_bits=(int) (bct->logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd))) + EPSILON);
    addr_bits_routed = (int) (bct->logtwo( (double)((double)C / (double)B)) + EPSILON);

    //rows = C/(8*B*A*Ndbl*Nspd);
    //cols = CHUNKSIZE*B*A*Nspd/Ndwl;

    rows = (int) (C / (8 * B * A * Ndbl * Nspd) + EPSILON);
    cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);

    numstack =
        (int) ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ndbl * Nspd))));
    if (numstack == 0) numstack = 1;

    if (numstack > 5) numstack = 5;

    /*dt: The *8 is there because above we mysteriously divide the capacity in BYTES by the number of BITS per wordline */
    l_predec_nor_v = rows * 8;
    /*dt: If we follow the original drawings from the TR's, then there is almost no horizontal wires, only the poly for contacting
    the nor gates. The poly part we don't model right now */
    l_predec_nor_h = 0;


    /* Calculate rise time.  Consider two inverters */


    if (NSubbanks > 2) {
        Ceq = bct->draincap(circuit_param_t::cparam_Waddrdrvp1, PCH, 1) + bct->draincap(circuit_param_t::cparam_Waddrdrvn1, NCH, 1) +
              bct->gatecap(circuit_param_t::cparam_Wdecdrivep_first + circuit_param_t::cparam_Wdecdriven_first, 0.0);

        tf = Ceq * bct->transreson(circuit_param_t::cparam_Waddrdrvn1, NCH, 1);
        nextinputtime = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                        (VTHINV360x240);
    }
    else {
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep_first, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven_first, NCH, 1) +
              bct->gatecap(circuit_param_t::cparam_Wdecdrivep_first + circuit_param_t::cparam_Wdecdriven_first, 0.0);

        tf = Ceq * bct->transreson(circuit_param_t::cparam_Wdecdriven_first, NCH, 1);
        nextinputtime = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                        (VTHINV360x240);
    }


    lkgCurrent += addr_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wdecdriven_first, circuit_param_t::cparam_Wdecdrivep_first, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp)
                  * 1.0 / (Ndwl * Ndbl);

    *Tdecdrive = 0;

    /*dt: the first inverter driving a bigger inverter*/
    Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep_first, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven_first, NCH, 1) +
          bct->gatecap(circuit_param_t::cparam_Wdecdrivep_second + circuit_param_t::cparam_Wdecdriven_second, 0.0);

    tf = Ceq * bct->transreson(circuit_param_t::cparam_Wdecdriven_first, NCH, 1);

    this_delay = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, RISE);

    *Tdecdrive += this_delay;
    inrisetime = this_delay / (1.0 - VTHINV360x240);


    if (circuit_param_t::cparam_nr_dectreesegments) {
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep_second, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven_second, NCH, 1) +
              bct->gatecap(3 * circuit_param_t::cparam_WdecdrivetreeN[circuit_param_t::cparam_nr_dectreesegments-1], 0) + circuit_param_t::cparam_Cdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1];
        Req = bct->transreson(circuit_param_t::cparam_Wdecdriven_second, NCH, 1) + circuit_param_t::cparam_Rdectreesegments[circuit_param_t::cparam_nr_dectreesegments-1];

        tf = Ceq * Req;
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);

        *Tdecdrive += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += addr_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += addr_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wdecdriven_second, circuit_param_t::cparam_Wdecdrivep_second, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp)
                      * 1.0 / (Ndwl * Ndbl);
    }



    /*dt: doing all the H-tree segments*/

    for (i = circuit_param_t::cparam_nr_dectreesegments; i > 2;i--) {
        /*dt: this too should alternate...*/
        Ceq = (circuit_param_t::cparam_Cdectreesegments[i-2] + bct->draincap(2 * circuit_param_t::cparam_WdecdrivetreeN[i-1], PCH, 1) + bct->draincap(circuit_param_t::cparam_WdecdrivetreeN[i-1], NCH, 1) +
               bct->gatecap(3 * circuit_param_t::cparam_WdecdrivetreeN[i-2], 0.0));
        Req = (circuit_param_t::cparam_Rdectreesegments[i-2] + bct->transreson(circuit_param_t::cparam_WdecdrivetreeN[i-1], NCH, 1));
        tf = Req * Ceq;
        /*dt: This shouldn't be all falling, but interleaved. Have to fix that at some point.*/
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);
        *Tdecdrive += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += addr_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ndwl * Ndbl) * pow((double) 2, (double) circuit_param_t::cparam_nr_dectreesegments - i) * addr_bits_routed * 0.5 *
                      cmos_ileakage(circuit_param_t::cparam_WdecdrivetreeN[i-1], 2 * circuit_param_t::cparam_WdecdrivetreeN[i-1], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }

    if (circuit_param_t::cparam_nr_dectreesegments) {
        Ceq = 4 * bct->gatecap(circuit_param_t::cparam_Wdec3to8n + circuit_param_t::cparam_Wdec3to8p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_Cdectreesegments[0] +
              bct->draincap(2 * circuit_param_t::cparam_WdecdrivetreeN[0], PCH, 1) + bct->draincap(circuit_param_t::cparam_WdecdrivetreeN[0], NCH, 1);
        Rwire = circuit_param_t::cparam_Rdectreesegments[0];
        tf = (Rwire + bct->transreson(2 * circuit_param_t::cparam_WdecdrivetreeN[0], PCH, 1)) * Ceq;

        dynPower += addr_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ndwl * Ndbl) * pow((double) 2, (double) circuit_param_t::cparam_nr_dectreesegments) * addr_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_WdecdrivetreeN[0], 2 * circuit_param_t::cparam_WdecdrivetreeN[0], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }
    else {
        Ceq = 4 * bct->gatecap(circuit_param_t::cparam_Wdec3to8n + circuit_param_t::cparam_Wdec3to8p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_Cdectreesegments[0] +
              bct->draincap(circuit_param_t::cparam_Wdecdrivep_second, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven_second, NCH, 1);
        Rwire = circuit_param_t::cparam_Rdectreesegments[0];
        tf = (Rwire + bct->transreson(circuit_param_t::cparam_Wdecdrivep_second, PCH, 1)) * Ceq;

        dynPower += addr_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ndwl * Ndbl) * addr_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wdecdriven_second, circuit_param_t::cparam_Wdecdrivep_second, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }

    // there are 8 nand gates in each 3-8 decoder. since these transistors are
    // stacked, we use a stacking factor of 1/5 (0.2). 0.5 signifies that we
    // are taking the average of both nmos and pmos transistors.



    this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHNAND60x120, FALL);
    *Tdecdrive += this_delay;

    lkgCurrent += 8 * 0.2 * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wdec3to8n, circuit_param_t::cparam_Wdec3to8p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp) *
                  // For the all the 3-8 decoders per quad:
                  ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ndbl * Nspd))))
                  /*for all the quads*/
                  *0.25;

    nextinputtime = this_delay / VTHNAND60x120;

    Ceq = 3 * bct->draincap(circuit_param_t::cparam_Wdec3to8p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdec3to8n, NCH, 3) +
          bct->gatecap(circuit_param_t::cparam_WdecNORn + circuit_param_t::cparam_WdecNORp, ((numstack * 40 / circuit_param_t::cparam_FUDGEFACTOR) + 20.0 / circuit_param_t::cparam_FUDGEFACTOR)) * rows / 8 +
          circuit_param_t::cparam_GlobalCbitmetal * (l_predec_nor_v) + circuit_param_t::cparam_GlobalCwordmetal * (l_predec_nor_h);
    Rwire = circuit_param_t::cparam_GlobalRbitmetal * (l_predec_nor_v) / 2 + circuit_param_t::cparam_GlobalRwordmetal * (l_predec_nor_h) / 2;

    tf = Ceq * (Rwire + bct->transreson(circuit_param_t::cparam_Wdec3to8n, NCH, 3));

    // 0.2 is the stacking factor, 0.5 for averging of nmos and pmos leakage
    // and since there are rows number of nor gates:

    lkgCurrent += 0.5 * 0.2 * rows * cmos_ileakage(circuit_param_t::cparam_WdecNORn, circuit_param_t::cparam_WdecNORp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    // number of active blocks among Ndwl modules
    if (Ndwl / Nspd < 1) {
        Nact = 1;
    }
    else {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //Nact = Ndwl/Nspd;
        Nact = (int) (Ndwl / Nspd + EPSILON);
    }

    //dynPower+=Ndwl*Ndbl*Ceq*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*4*ceil((1.0/3.0)*bct->logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd))));
    dynPower += 0.5 * Nact * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 4 * ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ndbl * Nspd))));

    /* we only want to charge the output to the threshold of the
       nor gate.  But the threshold depends on the number of inputs
       to the nor.  */

    switch (numstack) {
    case 1:
        vth = VTHNOR12x4x1; break;
    case 2:
        vth = VTHNOR12x4x2; break;
    case 3:
        vth = VTHNOR12x4x3; break;
    case 4:
        vth = VTHNOR12x4x4; break;
    case 5:
        vth = VTHNOR12x4x4; break;
    default:
        printf("error:numstack=%d\n", numstack);
        printf("Cacti does not support a series stack of %d transistors !\n", numstack);
        exit(0);
        break;
    }

    *Tdecoder1 = bct->horowitz(nextinputtime, tf, VTHNAND60x120, vth, RISE);

    nextinputtime = *Tdecoder1 / (1.0 - vth);

    /* Final stage: driving an inverter with the nor */

    Req = bct->transreson(circuit_param_t::cparam_WdecNORp, PCH, numstack);

    Ceq = (bct->gatecap(circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
           numstack * bct->draincap(circuit_param_t::cparam_WdecNORn, NCH, 1) + bct->draincap(circuit_param_t::cparam_WdecNORp, PCH, numstack));


    lkgCurrent += 0.5 * rows * cmos_ileakage(circuit_param_t::cparam_Wdecinvn, circuit_param_t::cparam_Wdecinvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    tf = Req * Ceq;

    *Tdecoder2 = bct->horowitz(nextinputtime, tf, vth, VSINV, FALL);

    *outrisetime = *Tdecoder2 / (VSINV);
    *nor_inputs = numstack;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    //printf("%g %g %g %d %d %d\n",*Tdecdrive,*Tdecoder1,*Tdecoder2,Ndwl, Ndbl,Nspd);

    //fprintf(stderr, "%f %f %f %f %d %d %d\n", (*Tdecdrive+*Tdecoder1+*Tdecoder2)*1e3, *Tdecdrive*1e3, *Tdecoder1*1e3, *Tdecoder2*1e3, Ndwl, Ndbl, Nspd);
    power->readOp.dynamic = dynPower;
    power->readOp.leakage = (lkgCurrent * circuit_param_t::cparam_VddPow) * Ndwl * Ndbl;

    power->writeOp.dynamic = dynPower;
    power->writeOp.leakage = (lkgCurrent * circuit_param_t::cparam_VddPow) * Ndwl * Ndbl;


    return(*Tdecdrive + *Tdecoder1 + *Tdecoder2);
}

/*----------------------------------------------------------------------*/

/* Decoder delay in the tag array (see section 6.1 of tech report) */
/*dt: incorporating leakage code from eCacti, see decoder_delay for more comments */
double cacti_time_t::decoder_tag_delay(int C, int B, int A, int Ntwl, int Ntbl, int Ntspd, double NSubbanks,
                                       double *Tdecdrive, double *Tdecoder1, double *Tdecoder2, double inrisetime, double *outrisetime, int *nor_inputs, powerDef *power)
{
    //double Ceq,Req,Rwire,tf,nextinputtime,vth,tstep;
    double Ceq, Req, Rwire, tf, nextinputtime, vth;
    int numstack, tagbits, Nact;
    int rows, cols;
    //int l_inv_predecode,l_predec_nor_v,l_predec_nor_h;
    int l_predec_nor_v, l_predec_nor_h;
    //double wire_cap, wire_res;
    double lkgCurrent = 0.0, dynPower = 0.0;
    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //int addr_bits=bct->logtwo( (double)((double)C/(double)(B*A*Ntbl*Ntspd)));
    double this_delay;
    int i = 0;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //int routing_bits = bct->logtwo( (double)((double)C/(double)B));
    int routing_bits = (int) (bct->logtwo( (double)((double)C / (double)B)) + EPSILON);
    int tag_bits_routed;

    rows = C / (8 * B * A * Ntbl * Ntspd);
    if (!force_tag) {
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS-bct->logtwo((double)C)+bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - (int)(bct->logtwo(NSubbanks)) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    tag_bits_routed = routing_bits + tagbits;


    cols = tagbits * A * Ntspd / Ntwl ;

    numstack =
        (int)ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ntbl * Ntspd))));
    if (numstack == 0) numstack = 1;
    if (numstack > 5) numstack = 5;

    /*dt: see comments in compute_device_widths*/
    /*dt: The *8 is there because above we mysteriously divide the capacity in BYTES by the number of BITS per wordline */
    l_predec_nor_v = rows * 8;
    /*dt: If we follow the original drawings from the TR's, then there is almost no horizontal wires, only the poly for contacting
    the nor gates. The poly part we don't model right now */
    l_predec_nor_h = 0;


    /* Calculate rise time.  Consider two inverters */
    if (NSubbanks > 2) {
        Ceq = bct->draincap(circuit_param_t::cparam_Waddrdrvp1, PCH, 1) + bct->draincap(circuit_param_t::cparam_Waddrdrvn1, NCH, 1) +
              bct->gatecap(circuit_param_t::cparam_Wtdecdrivep_first + circuit_param_t::cparam_Wtdecdriven_first, 0.0);

        tf = Ceq * bct->transreson(circuit_param_t::cparam_Waddrdrvn1, NCH, 1);

        nextinputtime = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                        (VTHINV360x240);
    }
    else {
        Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
              bct->gatecap(circuit_param_t::cparam_Wtdecdrivep_first + circuit_param_t::cparam_Wtdecdriven_first, 0.0);

        tf = Ceq * bct->transreson(circuit_param_t::cparam_Wdecdriven_first, NCH, 1);

        nextinputtime = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                        (VTHINV360x240);
    }

    lkgCurrent = 0.5 * cmos_ileakage(circuit_param_t::cparam_Wtdecdriven_first, circuit_param_t::cparam_Wtdecdrivep_first, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp) * 1.0 / (Ntwl * Ntbl);

    *Tdecdrive = 0;

    /*dt: the first inverter driving a bigger inverter*/
    Ceq = bct->draincap(circuit_param_t::cparam_Wtdecdrivep_first, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wtdecdriven_first, NCH, 1) +
          bct->gatecap(circuit_param_t::cparam_Wtdecdrivep_second + circuit_param_t::cparam_Wtdecdriven_second, 0.0);

    tf = Ceq * bct->transreson(circuit_param_t::cparam_Wtdecdriven_first, NCH, 1);

    this_delay = bct->horowitz(0.0, tf, VTHINV360x240, VTHINV360x240, RISE);

    *Tdecdrive += this_delay;
    inrisetime = this_delay / (1.0 - VTHINV360x240);


    if (circuit_param_t::cparam_nr_tdectreesegments) {
        Ceq = bct->draincap(circuit_param_t::cparam_Wtdecdrivep_second, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wtdecdriven_second, NCH, 1) +
              bct->gatecap(3 * circuit_param_t::cparam_WtdecdrivetreeN[circuit_param_t::cparam_nr_tdectreesegments-1], 0) + circuit_param_t::cparam_Ctdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1];
        Req = bct->transreson(circuit_param_t::cparam_Wtdecdriven_second, NCH, 1) + circuit_param_t::cparam_Rtdectreesegments[circuit_param_t::cparam_nr_tdectreesegments-1];

        tf = Ceq * Req;
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);

        *Tdecdrive += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += tag_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += tag_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wtdecdriven_second, circuit_param_t::cparam_Wtdecdrivep_second, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp)
                      * 1.0 / (Ntwl * Ntbl);
    }



    /*dt: doing all the H-tree segments*/

    for (i = circuit_param_t::cparam_nr_tdectreesegments; i > 2;i--) {
        /*dt: this too should alternate...*/
        Ceq = (circuit_param_t::cparam_Ctdectreesegments[i-2] + bct->draincap(2 * circuit_param_t::cparam_WtdecdrivetreeN[i-1], PCH, 1) + bct->draincap(circuit_param_t::cparam_WtdecdrivetreeN[i-1], NCH, 1) +
               bct->gatecap(3 * circuit_param_t::cparam_WtdecdrivetreeN[i-2], 0.0));
        Req = (circuit_param_t::cparam_Rtdectreesegments[i-2] + bct->transreson(circuit_param_t::cparam_WtdecdrivetreeN[i-1], NCH, 1));
        tf = Req * Ceq;
        /*dt: This shouldn't be all falling, but interleaved. Have to fix that at some point.*/
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);
        *Tdecdrive += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += tag_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ntwl * Ntbl) * pow(2.00, (double) circuit_param_t::cparam_nr_tdectreesegments - i) * tag_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_WtdecdrivetreeN[i-1], 2 * circuit_param_t::cparam_WtdecdrivetreeN[i-1], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    }

    if (circuit_param_t::cparam_nr_tdectreesegments) {
        //v4.1: Change below, bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wdec3to8p,10.0) -> bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wtdec3to8p,10.0)
        //Ceq = 4*bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wdec3to8p,10.0) + circuit_param_t::cparam_Ctdectreesegments[0] +
        //bct->draincap(2*circuit_param_t::cparam_WtdecdrivetreeN[0],PCH,1)+ bct->draincap(circuit_param_t::cparam_WtdecdrivetreeN[0],NCH,1);
        Ceq = 4 * bct->gatecap(circuit_param_t::cparam_Wtdec3to8n + circuit_param_t::cparam_Wtdec3to8p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_Ctdectreesegments[0] +
              bct->draincap(2 * circuit_param_t::cparam_WtdecdrivetreeN[0], PCH, 1) + bct->draincap(circuit_param_t::cparam_WtdecdrivetreeN[0], NCH, 1);
        Rwire = circuit_param_t::cparam_Rtdectreesegments[0];
        tf = (Rwire + bct->transreson(2 * circuit_param_t::cparam_WtdecdrivetreeN[0], PCH, 1)) * Ceq;

        dynPower += tag_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ntwl * Ntbl) * pow(2.00 , (double)circuit_param_t::cparam_nr_tdectreesegments) * tag_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_WtdecdrivetreeN[0], 2 * circuit_param_t::cparam_WtdecdrivetreeN[0], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }
    else {
        //v4.1: Change below, bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wdec3to8p,10.0) -> bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wtdec3to8p,10.0)
        //Ceq = 4*bct->gatecap(circuit_param_t::cparam_Wtdec3to8n+circuit_param_t::cparam_Wdec3to8p,10.0) + circuit_param_t::cparam_Ctdectreesegments[0] +
        //bct->draincap(circuit_param_t::cparam_Wtdecdrivep_second,PCH,1)+ bct->draincap(circuit_param_t::cparam_Wtdecdriven_second,NCH,1);
        Ceq = 4 * bct->gatecap(circuit_param_t::cparam_Wtdec3to8n + circuit_param_t::cparam_Wtdec3to8p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_Ctdectreesegments[0] +
              bct->draincap(circuit_param_t::cparam_Wtdecdrivep_second, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wtdecdriven_second, NCH, 1);
        Rwire = circuit_param_t::cparam_Rtdectreesegments[0];
        tf = (Rwire + bct->transreson(circuit_param_t::cparam_Wtdecdrivep_second, PCH, 1)) * Ceq;

        dynPower += tag_bits_routed * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += 1.0 / (Ntwl * Ntbl) * tag_bits_routed * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wtdecdriven_second, circuit_param_t::cparam_Wtdecdrivep_second, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }
    this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHNAND60x120, FALL);
    *Tdecdrive += this_delay;
    nextinputtime = this_delay / VTHNAND60x120;

    // there are 8 nand gates in each 3-8 decoder. since these transistors are
    // stacked, we use a stacking factor of 1/5 (0.2). 0.5 signifies that we
    // are taking the average of both nmos and pmos transistors.

    lkgCurrent += 8 * 0.2 * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wtdec3to8n, circuit_param_t::cparam_Wtdec3to8p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp) *
                  // For the all the 3-8 decoders per quad:
                  ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ntbl * Ntspd))))
                  /*for all the quads*/
                  *0.25;


    /* second stage: driving a bunch of nor gates with a nand */


    Ceq = 3 * bct->draincap(circuit_param_t::cparam_Wtdec3to8p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wtdec3to8n, NCH, 3) +
          bct->gatecap(circuit_param_t::cparam_WtdecNORn + circuit_param_t::cparam_WtdecNORp, ((numstack * 40) / circuit_param_t::cparam_FUDGEFACTOR + 20.0 / circuit_param_t::cparam_FUDGEFACTOR)) * rows +
          circuit_param_t::cparam_GlobalCbitmetal * (l_predec_nor_v) + circuit_param_t::cparam_GlobalCwordmetal * (l_predec_nor_h);

    Rwire = circuit_param_t::cparam_GlobalRbitmetal * (l_predec_nor_v) / 2 + circuit_param_t::cparam_GlobalRwordmetal * (l_predec_nor_h) / 2;

    tf = Ceq * (Rwire + bct->transreson(circuit_param_t::cparam_Wtdec3to8n, NCH, 3));

    // 0.2 is the stacking factor, 0.5 for averging of nmos and pmos leakage
    // and since there are rows number of nor gates:

    lkgCurrent += 0.5 * 0.2 * rows * cmos_ileakage(circuit_param_t::cparam_WtdecNORn, circuit_param_t::cparam_WtdecNORp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    // Number of active blocks in Ntwl modules
    if (Ntwl / Ntspd < 1) {
        Nact = 1;
    }
    else {
        Nact = Ntwl / Ntspd;
    }

    dynPower += 0.5 * Nact * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 4 * ceil((1.0 / 3.0) * bct->logtwo( (double)((double)C / (double)(B * A * Ntbl * Ntspd))));
    //dynPower+=Ntwl*Ntbl*Ceq*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*4*ceil((1.0/3.0)*bct->logtwo( (double)((double)C/(double)(B*A*Ntbl*Ntspd))));

    /* we only want to charge the output to the threshold of the
       nor gate.  But the threshold depends on the number of inputs
       to the nor.  */

    switch (numstack) {
    case 1:
        vth = VTHNOR12x4x1; break;
    case 2:
        vth = VTHNOR12x4x2; break;
    case 3:
        vth = VTHNOR12x4x3; break;
    case 4:
        vth = VTHNOR12x4x4; break;
    case 5:
        vth = VTHNOR12x4x4; break;
    case 6:
        vth = VTHNOR12x4x4; break;
    default:
        printf("error:numstack=%d\n", numstack);
        printf("Cacti does not support a series stack of %d transistors !\n", numstack);
        exit(0);
        break;
    }

    *Tdecoder1 = bct->horowitz(nextinputtime, tf, VTHNAND60x120, vth, RISE);
    nextinputtime = *Tdecoder1 / (1.0 - vth);

    /* Final stage: driving an inverter with the nor */

    Req = bct->transreson(circuit_param_t::cparam_WtdecNORp, PCH, numstack);
    Ceq = (bct->gatecap(circuit_param_t::cparam_Wtdecinvn + circuit_param_t::cparam_Wtdecinvp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) + numstack * bct->draincap(circuit_param_t::cparam_WtdecNORn, NCH, 1) +
           bct->draincap(circuit_param_t::cparam_WtdecNORp, PCH, numstack));

    tf = Req * Ceq;
    *Tdecoder2 = bct->horowitz(nextinputtime, tf, vth, VSINV, FALL);
    *outrisetime = *Tdecoder2 / (VSINV);
    *nor_inputs = numstack;

    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent += 0.5 * rows * cmos_ileakage(circuit_param_t::cparam_Wtdecinvn, circuit_param_t::cparam_Wtdecinvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    power->readOp.dynamic = dynPower;
    power->readOp.leakage = (lkgCurrent * circuit_param_t::cparam_VddPow) * Ntwl * Ntbl;

    power->writeOp.dynamic = dynPower;
    power->writeOp.leakage = (lkgCurrent * circuit_param_t::cparam_VddPow) * Ntwl * Ntbl;

    return(*Tdecdrive + *Tdecoder1 + *Tdecoder2);
}

/*dt: still have to add leakage and gate width calc to this function */
double cacti_time_t::fa_tag_delay (int C, int B, int Ntwl, int Ntbl, int Ntspd,
                                   double *Tagdrive, double *Tag1, double *Tag2, double *Tag3, double *Tag4, double *Tag5, double *outrisetime,
                                   int *nor_inputs,
                                   powerDef *power)
{
    //double Ceq, Req, Rwire, rows, tf, nextinputtime, vth, tstep;
    double Ceq, Req, Rwire, rows, tf, nextinputtime;
    //int numstack;
    double Tagdrive1 = 0, Tagdrive2 = 0;
    double Tagdrive0a = 0, Tagdrive0b = 0;
    double TagdriveA = 0, TagdriveB = 0;
    double TagdriveA1 = 0, TagdriveB1 = 0;
    double TagdriveA2 = 0, TagdriveB2 = 0;

    double dynPower = 0.0;

    rows = C / (B * Ntbl);

    /* Calculate rise time.  Consider two inverters */

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wdecdriven, NCH, 1);
    nextinputtime = bct->horowitz (0.0, tf, VTHINV360x240, VTHINV360x240, FALL) /
                    (VTHINV360x240);

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecdrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecdriven, NCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Wdecdrivep + circuit_param_t::cparam_Wdecdriven, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wdecdrivep, PCH, 1);
    nextinputtime = bct->horowitz (nextinputtime, tf, VTHINV360x240, VTHINV360x240,
                                   RISE) / (1.0 - VTHINV360x240);

    // If tag bitline divisions, add extra driver

    if (Ntbl > 1)
    {
        Ceq = bct->draincap(circuit_param_t::cparam_Wfadrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadriven, NCH, 1) +
              bct->gatecap (circuit_param_t::cparam_Wfadrive2p + circuit_param_t::cparam_Wfadrive2n, 0.0);
        tf = Ceq * bct->transreson (circuit_param_t::cparam_Wfadriven, NCH, 1);
        TagdriveA = bct->horowitz (nextinputtime, tf, VSINV, VSINV, FALL);
        nextinputtime = TagdriveA / (VSINV);
        dynPower += .5 * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS;

        if (Ntbl <= 4)
        {
            Ceq =
                bct->draincap(circuit_param_t::cparam_Wfadrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadrive2n, NCH,
                        1) +
                bct->gatecap (circuit_param_t::cparam_Wfadecdrive1p + circuit_param_t::cparam_Wfadecdrive1n,
                              10.0 / circuit_param_t::cparam_FUDGEFACTOR) * 2 + + circuit_param_t::cparam_FACwordmetal * sqrt ((rows +
                                      1) * Ntbl) / 2 +
                circuit_param_t::cparam_FACbitmetal * sqrt ((rows + 1) * Ntbl) / 2;
            Rwire =
                circuit_param_t::cparam_FARwordmetal * sqrt ((rows + 1) * Ntbl) * .5 / 2 +
                circuit_param_t::cparam_FARbitmetal * sqrt ((rows + 1) * Ntbl) * .5 / 2;
            tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wfadrive2p, PCH, 1) + Rwire);
            TagdriveB = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
            nextinputtime = TagdriveB / (1.0 - VSINV);
            dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * 2;
        }
        else
        {
            Ceq =
                bct->draincap(circuit_param_t::cparam_Wfadrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadrive2n, NCH,
                        1) +
                bct->gatecap (circuit_param_t::cparam_Wfadrivep + circuit_param_t::cparam_Wfadriven,
                              10.0 / circuit_param_t::cparam_FUDGEFACTOR) * 2 + + circuit_param_t::cparam_FACwordmetal * sqrt ((rows +
                                      1) * Ntbl) / 2 +
                circuit_param_t::cparam_FACbitmetal * sqrt ((rows + 1) * Ntbl) / 2;
            Rwire =
                circuit_param_t::cparam_FARwordmetal * sqrt ((rows + 1) * Ntbl) * .5 / 2 +
                circuit_param_t::cparam_FARbitmetal * sqrt ((rows + 1) * Ntbl) * .5 / 2;
            tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wfadrive2p, PCH, 1) + Rwire);
            TagdriveB = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
            nextinputtime = TagdriveB / (1.0 - VSINV);
            dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * 4;

            Ceq = bct->draincap(circuit_param_t::cparam_Wfadrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadriven, NCH, 1) +
                  bct->gatecap (circuit_param_t::cparam_Wfadrive2p + circuit_param_t::cparam_Wfadrive2n, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
            tf = Ceq * bct->transreson (circuit_param_t::cparam_Wfadriven, NCH, 1);
            TagdriveA1 = bct->horowitz (nextinputtime, tf, VSINV, VSINV, FALL);
            nextinputtime = TagdriveA1 / (VSINV);
            dynPower += .5 * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS;

            if (Ntbl <= 16)
            {
                Ceq =
                    bct->draincap(circuit_param_t::cparam_Wfadrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadrive2n, NCH,
                            1) +
                    bct->gatecap (circuit_param_t::cparam_Wfadecdrive1p + circuit_param_t::cparam_Wfadecdrive1n,
                                  10.0 / circuit_param_t::cparam_FUDGEFACTOR) * 2 + + circuit_param_t::cparam_FACwordmetal * sqrt ((rows +
                                          1) * Ntbl) / 4 +
                    circuit_param_t::cparam_FACbitmetal * sqrt ((rows + 1) * Ntbl) / 4;
                Rwire =
                    circuit_param_t::cparam_FARwordmetal * sqrt ((rows + 1) * Ntbl) * .5 / 4 +
                    circuit_param_t::cparam_FARbitmetal * sqrt ((rows + 1) * Ntbl) * .5 / 4;
                tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wfadrive2p, PCH, 1) + Rwire);
                TagdriveB1 = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
                nextinputtime = TagdriveB1 / (1.0 - VSINV);
                dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * 8;
            }
            else
            {
                Ceq =
                    bct->draincap(circuit_param_t::cparam_Wfadrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadrive2n, NCH,
                            1) +
                    bct->gatecap (circuit_param_t::cparam_Wfadrivep + circuit_param_t::cparam_Wfadriven,
                                  10.0 / circuit_param_t::cparam_FUDGEFACTOR) * 2 + + circuit_param_t::cparam_FACwordmetal * sqrt ((rows +
                                          1) * Ntbl) / 4 +
                    circuit_param_t::cparam_FACbitmetal * sqrt ((rows + 1) * Ntbl) / 4;
                Rwire =
                    circuit_param_t::cparam_FARwordmetal * sqrt ((rows + 1) * Ntbl) * .5 / 4 +
                    circuit_param_t::cparam_FARbitmetal * sqrt ((rows + 1) * Ntbl) * .5 / 4;
                tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wfadrive2p, PCH, 1) + Rwire);
                TagdriveB1 = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
                nextinputtime = TagdriveB1 / (1.0 - VSINV);
                dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * 8;

                Ceq =
                    bct->draincap(circuit_param_t::cparam_Wfadrivep, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadriven, NCH,
                            1) +
                    bct->gatecap (circuit_param_t::cparam_Wfadrive2p + circuit_param_t::cparam_Wfadrive2n, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
                tf = Ceq * bct->transreson (circuit_param_t::cparam_Wfadriven, NCH, 1);
                TagdriveA2 = bct->horowitz (nextinputtime, tf, VSINV, VSINV, FALL);
                nextinputtime = TagdriveA2 / (VSINV);
                dynPower += .5 * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS;

                Ceq =
                    bct->draincap(circuit_param_t::cparam_Wfadrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadrive2n, NCH,
                            1) +
                    bct->gatecap (circuit_param_t::cparam_Wfadecdrive1p + circuit_param_t::cparam_Wfadecdrive1n,
                                  10.0 / circuit_param_t::cparam_FUDGEFACTOR) * 2 + + circuit_param_t::cparam_FACwordmetal * sqrt ((rows +
                                          1) * Ntbl) / 8 +
                    circuit_param_t::cparam_FACbitmetal * sqrt ((rows + 1) * Ntbl) / 8;
                Rwire =
                    circuit_param_t::cparam_FARwordmetal * sqrt ((rows + 1) * Ntbl) * .5 / 8 +
                    circuit_param_t::cparam_FARbitmetal * sqrt ((rows + 1) * Ntbl) * .5 / 8;
                tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wfadrive2p, PCH, 1) + Rwire);
                TagdriveB2 = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
                nextinputtime = TagdriveB2 / (1.0 - VSINV);
                dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * 16;
            }
        }
    }

    /* Two more inverters for enable delay */

    Ceq = bct->draincap(circuit_param_t::cparam_Wfadecdrive1p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadecdrive1n, NCH, 1)
          + bct->gatecap (circuit_param_t::cparam_Wfadecdrive2p + circuit_param_t::cparam_Wfadecdrive2n, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wfadecdrive1n, NCH, 1);
    Tagdrive0a = bct->horowitz (nextinputtime, tf, VSINV, VSINV, FALL);
    nextinputtime = Tagdrive0a / (VSINV);
    dynPower += .5 * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * Ntbl;

    Ceq = bct->draincap(circuit_param_t::cparam_Wfadecdrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadecdrive2n, NCH, 1) +
          + bct->gatecap (circuit_param_t::cparam_Wfadecdrivep + circuit_param_t::cparam_Wfadecdriven, 10.0 / circuit_param_t::cparam_FUDGEFACTOR)
          + bct->gatecap (circuit_param_t::cparam_Wfadecdrive2p + circuit_param_t::cparam_Wfadecdrive2n, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wfadecdrive2p, PCH, 1);
    Tagdrive0b = bct->horowitz (nextinputtime, tf, VSINV, VSINV, RISE);
    nextinputtime = Tagdrive0b / (VSINV);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * Ntbl;

    /* First stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Wfadecdrive2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wfadecdrive2n, NCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Wfadecdrivep + circuit_param_t::cparam_Wfadecdriven, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf = Ceq * bct->transresswitch (circuit_param_t::cparam_Wfadecdrive2n, NCH, 1);
    Tagdrive1 = bct->horowitz (nextinputtime, tf, VSINV, VTHFA1, FALL);
    nextinputtime = Tagdrive1 / VTHFA1;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * .5 * Ntbl;

    Ceq = bct->draincap(circuit_param_t::cparam_Wfadecdrivep, PCH, 2) + bct->draincap(circuit_param_t::cparam_Wfadecdriven, NCH, 2)
          + bct->draincap(circuit_param_t::cparam_Wfaprechn, NCH, 1)
          + bct->gatecap (circuit_param_t::cparam_Wdummyn, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) * (rows + 1) + circuit_param_t::cparam_FACbitmetal * (rows + 1);

    Rwire = circuit_param_t::cparam_FARbitmetal * (rows + 1) * .5;
    tf = (Rwire + bct->transreson (circuit_param_t::cparam_Wfadecdrivep, PCH, 1) +
          bct->transresswitch (circuit_param_t::cparam_Wfadecdrivep, PCH, 1)) * Ceq;
    Tagdrive2 = bct->horowitz (nextinputtime, tf, VTHFA1, VTHFA2, RISE);
    nextinputtime = Tagdrive2 / (1 - VTHFA2);

    *Tagdrive =
        Tagdrive1 + Tagdrive2 + TagdriveA + TagdriveB + TagdriveA1 + TagdriveA2 +
        TagdriveB1 + TagdriveB2 + Tagdrive0a + Tagdrive0b;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * ADDRESS_BITS * Ntbl;

    /* second stage */

    Ceq = .5 * ADDRESS_BITS * 2 * bct->draincap(circuit_param_t::cparam_Wdummyn, NCH, 2)
          + bct->draincap(circuit_param_t::cparam_Wfaprechp, PCH, 1)
          + bct->gatecap (circuit_param_t::cparam_Waddrnandn + circuit_param_t::cparam_Waddrnandp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) + circuit_param_t::cparam_FACwordmetal * ADDRESS_BITS;
    Rwire = circuit_param_t::cparam_FARwordmetal * ADDRESS_BITS * .5;
    tf =
        Ceq * (Rwire + bct->transreson (circuit_param_t::cparam_Wdummyn, NCH, 1) +
               bct->transreson (circuit_param_t::cparam_Wdummyn, NCH, 1));
    *Tag1 = bct->horowitz (nextinputtime, tf, VTHFA2, VTHFA3, FALL);
    nextinputtime = *Tag1 / VTHFA3;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * rows * Ntbl;

    /* third stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Waddrnandn, NCH, 2)
          + 2 * bct->draincap(circuit_param_t::cparam_Waddrnandp, PCH, 1)
          + bct->gatecap (circuit_param_t::cparam_Wdummyinvn + circuit_param_t::cparam_Wdummyinvp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf = Ceq * (bct->transresswitch (circuit_param_t::cparam_Waddrnandp, PCH, 1));
    *Tag2 = bct->horowitz (nextinputtime, tf, VTHFA3, VTHFA4, RISE);
    nextinputtime = *Tag2 / (1 - VTHFA4);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * rows * Ntbl;

    /* fourth stage */

    Ceq = (rows) * (bct->gatecap (circuit_param_t::cparam_Wfanorn + circuit_param_t::cparam_Wfanorp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR))
          + bct->draincap(circuit_param_t::cparam_Wdummyinvn, NCH, 1)
          + bct->draincap(circuit_param_t::cparam_Wdummyinvp, PCH, 1) + circuit_param_t::cparam_FACbitmetal * rows;
    Rwire = circuit_param_t::cparam_FARbitmetal * rows * .5;
    Req = Rwire + bct->transreson (circuit_param_t::cparam_Wdummyinvn, NCH, 1);
    tf = Req * Ceq;
    *Tag3 = bct->horowitz (nextinputtime, tf, VTHFA4, VTHFA5, FALL);
    *outrisetime = *Tag3 / VTHFA5;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * Ntbl;

    /* fifth stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Wfanorp, PCH, 2)
          + 2 * bct->draincap(circuit_param_t::cparam_Wfanorn, NCH, 1) + bct->gatecap (circuit_param_t::cparam_Wfainvn + circuit_param_t::cparam_Wfainvp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf =
        Ceq * (bct->transresswitch (circuit_param_t::cparam_Wfanorp, PCH, 1) + bct->transreson (circuit_param_t::cparam_Wfanorp, PCH, 1));
    *Tag4 = bct->horowitz (nextinputtime, tf, VTHFA5, VTHFA6, RISE);
    nextinputtime = *Tag4 / (1 - VTHFA6);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    /* final stage */

    Ceq = (bct->gatecap (circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
           + bct->draincap(circuit_param_t::cparam_Wfainvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wfainvp, PCH, 1));
    Req = bct->transresswitch (circuit_param_t::cparam_Wfainvn, NCH, 1);
    tf = Req * Ceq;
    *Tag5 = bct->horowitz (nextinputtime, tf, VTHFA6, VSINV, FALL);
    *outrisetime = *Tag5 / VSINV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    //      if (Ntbl==32)
    //        fprintf(stderr," 1st - %f %f\n 2nd - %f %f\n 3rd - %f %f\n 4th - %f %f\n 5th - %f %f\nPD : %f\nNAND : %f\n INV : %f\n NOR : %f\n INV : %f\n", TagdriveA*1e9, TagdriveB*1e9, TagdriveA1*1e9, TagdriveB1*1e9, TagdriveA2*1e9, TagdriveB2*1e9, Tagdrive0a*1e9,Tagdrive0b*1e9,Tagdrive1*1e9, Tagdrive2*1e9, *Tag1*1e9, *Tag2*1e9, *Tag3*1e9, *Tag4*1e9, *Tag5*1e9);
    power->writeOp.dynamic = dynPower;
    power->readOp.dynamic = dynPower;
    return (*Tagdrive + *Tag1 + *Tag2 + *Tag3 + *Tag4 + *Tag5);
}


/*----------------------------------------------------------------------*/

/* Data array wordline delay (see section 6.2 of tech report) */

/*dt: incorporated leakage calc from eCacti*/
double cacti_time_t::wordline_delay (int C, int B, int A, int Ndwl, int Ndbl, double Nspd,
                                     double inrisetime,
                                     double *outrisetime, powerDef *power)
{
    //double tf, nextinputtime, Ceq, Req, Rline, Cline;
    double tf, nextinputtime, Ceq, Rline, Cline;
    int cols, Nact;
    double lkgCurrent = 0.0, dynPower = 0.0;
    double Tworddrivedel, Twordchargedel;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one

    //cols = CHUNKSIZE*B*A*Nspd/Ndwl;
    cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);

    /* Choose a transistor size that makes sense */
    /* Use a first-order approx */

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecinvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecinvp, PCH, 1) +
          bct->gatecap(circuit_param_t::cparam_WwlDrvp + circuit_param_t::cparam_WwlDrvn, 20.0 / circuit_param_t::cparam_FUDGEFACTOR);

    tf = bct->transreson(circuit_param_t::cparam_Wdecinvp, PCH, 1) * Ceq;

    // atleast one wordline driver in each block switches
    lkgCurrent = 0.5 * cmos_ileakage(circuit_param_t::cparam_WwlDrvn, circuit_param_t::cparam_WwlDrvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    Tworddrivedel = bct->horowitz(inrisetime, tf, VSINV, VSINV, RISE);
    nextinputtime = Tworddrivedel / (1.0 - VSINV);

    Cline = (bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             circuit_param_t::cparam_Cwordmetal) * cols +
            bct->draincap(circuit_param_t::cparam_WwlDrvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_WwlDrvp, PCH, 1);
    Rline = circuit_param_t::cparam_Rwordmetal * cols / 2;

    tf = (bct->transreson(circuit_param_t::cparam_WwlDrvp, PCH, 1) + Rline) * Cline;

    Twordchargedel = bct->horowitz(nextinputtime, tf, VSINV, VSINV, FALL);
    *outrisetime = Twordchargedel / VSINV;

    dynPower += Cline * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    //	fprintf(stderr, "%d %f %f\n", cols, Tworddrivedel*1e3, Twordchargedel*1e3);
    // Number of active blocks in Ndwl modules
    if (Ndwl / Nspd < 1) {
        Nact = 1;
    }
    else {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //Nact = Ndwl/Nspd;
        Nact = (int) (Ndwl / Nspd + EPSILON);
    }

    power->readOp.dynamic = dynPower * Nact;
    power->readOp.leakage = lkgCurrent * Ndwl * Ndbl * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = dynPower * Nact;
    power->writeOp.leakage = lkgCurrent * Ndwl * Ndbl * circuit_param_t::cparam_VddPow;
    // leakage power for the wordline driver to be accounted in the decoder module..
    return(Tworddrivedel + Twordchargedel);
}
/*----------------------------------------------------------------------*/

/* Tag array wordline delay (see section 6.3 of tech report) */

/*dt: incorporated leakage calc from eCacti*/
double cacti_time_t::wordline_tag_delay (int C, int B, int A,
        int Ntspd, int Ntwl, int Ntbl, double NSubbanks,
        double inrisetime, double *outrisetime, powerDef *power)
{
    double tf;
    double Cline, Rline, Ceq, nextinputtime;
    int tagbits, Nact;
    double lkgCurrent = 0.0, dynPower = 0.0;
    double Tworddrivedel, Twordchargedel;
    int cols;
    double Cload;


    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS-bct->logtwo((double)C)+bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - (int)(bct->logtwo(NSubbanks)) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    cols = tagbits * A * Ntspd / Ntwl;

    // capacitive load on the wordline - C_int + C_memCellLoad * NCells
    Cline = (bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) + circuit_param_t::cparam_TagCwordmetal) * cols;

    Cload = Cline / bct->gatecap(1.0, 0.0);
    // apprx width of the driver for optimal delay

    circuit_param_t::cparam_Wtdecinvn = Cload * SizingRatio / 3; circuit_param_t::cparam_Wtdecinvp = 2 * Cload * SizingRatio / 3;

    /* first stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Wtdecinvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wtdecinvp, PCH, 1) +
          bct->gatecap(circuit_param_t::cparam_WtwlDrvn + circuit_param_t::cparam_WtwlDrvp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf = bct->transreson(circuit_param_t::cparam_Wtdecinvn, NCH, 1) * Ceq;

    Tworddrivedel = bct->horowitz(inrisetime, tf, VSINV, VSINV, RISE);
    nextinputtime = Tworddrivedel / (1.0 - VSINV);

    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    /* second stage */
    Cline = (bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) +
             circuit_param_t::cparam_TagCwordmetal) * tagbits * A * Ntspd / Ntwl +
            bct->draincap(circuit_param_t::cparam_WtwlDrvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_WtwlDrvp, PCH, 1);
    Rline = circuit_param_t::cparam_TagRwordmetal * tagbits * A * Ntspd / (2 * Ntwl);
    tf = (bct->transreson(circuit_param_t::cparam_WtwlDrvp, PCH, 1) + Rline) * Cline;

    lkgCurrent = 0.5 * cmos_ileakage(circuit_param_t::cparam_WtwlDrvn, circuit_param_t::cparam_WtwlDrvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    Twordchargedel = bct->horowitz(nextinputtime, tf, VSINV, VSINV, FALL);
    *outrisetime = Twordchargedel / VSINV;

    dynPower += Cline * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    // Number of active blocks in Ntwl modules
    if (Ntwl / Ntspd < 1) {
        Nact = 1;
    }
    else {
        Nact = Ntwl / Ntspd;
    }


    power->readOp.dynamic = dynPower * Nact;
    power->readOp.leakage = lkgCurrent * Ntwl * Ntbl * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = dynPower * Nact;
    power->writeOp.leakage = lkgCurrent * Ntwl * Ntbl * circuit_param_t::cparam_VddPow;

    // leakage power for the wordline drivers to be accounted in the decoder module..
    return(Tworddrivedel + Twordchargedel);

}

/*----------------------------------------------------------------------*/

/* Data array bitline: (see section 6.4 in tech report) */

/*dt: integrated width calc and leakage from eCacti */
/* Tpre is precharge delay */
double cacti_time_t::bitline_delay (int C, int A, int B, int Ndwl, int Ndbl, double Nspd,
                                    double inrisetime,
                                    double *outrisetime, powerDef *power, double Tpre)
{
    //double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2, a, b, c;
    double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2;
    double m, tstep, Rpdrive;
    double Cbitrow;		/* bitline capacitance due to access transistor */
    int rows, cols, Nact;
    int muxway;
    double dynWrtEnergy = 0.0, dynRdEnergy = 0.0;
    double Icell, Iport;

    // leakage current of a memory bit-cell
    /*dt: access port transistors only NMOS of course, hence pWidth = 0 */
    Iport = cmos_ileakage(circuit_param_t::cparam_Wmemcella   , 0           , Vt_cell_nmos_high, Vthn, Vt_cell_pmos_high, Vthp);
    Icell = cmos_ileakage(circuit_param_t::cparam_Wmemcellnmos, circuit_param_t::cparam_Wmemcellpmos, Vt_cell_nmos_high, Vthn, Vt_cell_pmos_high, Vthp);

    // number of bitcells = Cache size in bytes * (8
    // ECC and other overhead)  = C*CHUNKSIZE
    // leakage current for the whole memory core -
    //
    Icell *= C * CHUNKSIZE;
    Iport *= C * CHUNKSIZE;

    Cbitrow = bct->draincap(circuit_param_t::cparam_Wmemcella, NCH, 1) / 2.0;	/* due to shared contact */
    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //rows = C / (B * A * Ndbl * Nspd);
    //cols = CHUNKSIZE * B * A * Nspd / Ndwl;
    rows = (int) (C / (B * A * Ndbl * Nspd) + EPSILON);
    cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);

    // Determine equivalent width of bitline precharge transistors
    // we assume that Precharge time = decoder delay + wordlineDriver delay time.
    // This is because decoder delay + wordline driver delay contributes to most
    // of the read/write access times.

    Cline = rows * (Cbitrow + circuit_param_t::cparam_Cbitmetal);
    Rpdrive = Tpre / (Cline * log(VSINV) * -1.0);
    circuit_param_t::cparam_Wbitpreequ = bct->restowidth(Rpdrive, PCH);

    // Note that circuit_param_t::cparam_Wbitpreequ is the equiv. pch transistor width
    circuit_param_t::cparam_Wpch = 2.0 / 3.0 * circuit_param_t::cparam_Wbitpreequ;

    if (circuit_param_t::cparam_Wpch > circuit_param_t::cparam_Wpchmax) {
        circuit_param_t::cparam_Wpch = circuit_param_t::cparam_Wpchmax;
    }

    //v4.1: Expressing all widths in terms of circuit_param_t::cparam_FEATURESIZE of input tech node

    // Isolation transistor width is set to 10 (typical). Its usually wide to reduce
    // resistance offered for transfer of bitline voltage to sense-amp.
    circuit_param_t::cparam_Wiso = 12.5 * circuit_param_t::cparam_FEATURESIZE;//was 10 micron for the 0.8 micron process

    // width of sense precharge
    // (depends on width of sense-amp transistors and output driver size)
    // ToDo: calculate the width of sense-amplifier as a function of access times...

    circuit_param_t::cparam_WsPch = 6.25 * circuit_param_t::cparam_FEATURESIZE; // ToDo: Determine widths of these devices analytically; was 5 micron for the 0.8 micron process
    circuit_param_t::cparam_WsenseN = 3.75 * circuit_param_t::cparam_FEATURESIZE; // sense amplifier N-trans; was 3 micron for the 0.8 micron process
    circuit_param_t::cparam_WsenseP = 7.5 * circuit_param_t::cparam_FEATURESIZE; // sense amplifier P-trans; was 6 micron for the 0.8 micron process
    circuit_param_t::cparam_WsenseEn = 5 * circuit_param_t::cparam_FEATURESIZE; // Sense enable transistor of the sense amplifier; was 4 micron for the 0.8 micron process
    circuit_param_t::cparam_WoBufP = 10 * circuit_param_t::cparam_FEATURESIZE; // output buffer corresponding to the sense amplifier; was 8 micron for the 0.8 micron process
    circuit_param_t::cparam_WoBufN = 5 * circuit_param_t::cparam_FEATURESIZE; //was 4 micron for the 0.8 micron process

    // ToDo: detemine the size of colmux (Wbitmux)

    if (8 * B / circuit_param_t::cparam_BITOUT == 1 && Ndbl * Nspd == 1)
    {
        Cline = rows * (Cbitrow + circuit_param_t::cparam_Cbitmetal) + 2 * bct->draincap(circuit_param_t::cparam_Wbitpreequ, PCH, 1);
        Ccolmux = bct->gatecap(circuit_param_t::cparam_WsenseN + circuit_param_t::cparam_WsenseP, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        Rlineb = circuit_param_t::cparam_Rbitmetal * rows / 2.0;
        r1 = Rlineb;
        // circuit_param_t::cparam_muxover=1;
    }
    else
    {
        if (Nspd > MAX_COL_MUX)
        {
            //circuit_param_t::cparam_muxover=8*B/circuit_param_t::cparam_BITOUT;
            muxway = MAX_COL_MUX;
        }
        else if (8 * B * Nspd / circuit_param_t::cparam_BITOUT > MAX_COL_MUX)
        {
            muxway = MAX_COL_MUX;
            // circuit_param_t::cparam_muxover=(8*B/circuit_param_t::cparam_BITOUT)/(MAX_COL_MUX/(Ndbl*Nspd));
        }
        else
        {
            muxway = 8 * B * Nspd / circuit_param_t::cparam_BITOUT;
            // circuit_param_t::cparam_muxover=1;
        }

        Cline = rows * (Cbitrow + circuit_param_t::cparam_Cbitmetal) + 2 * bct->draincap(circuit_param_t::cparam_Wbitpreequ, PCH,
                1) + bct->draincap(circuit_param_t::cparam_Wiso, PCH, 1);
        Ccolmux = muxway * (bct->draincap(circuit_param_t::cparam_Wiso, PCH, 1)) + bct->gatecap(circuit_param_t::cparam_WsenseN + circuit_param_t::cparam_WsenseP, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);

        Rlineb = circuit_param_t::cparam_Rbitmetal * rows / 2.0;
        r1 = Rlineb + bct->transreson(circuit_param_t::cparam_Wiso, PCH, 1);
    }
    r2 = bct->transreson (circuit_param_t::cparam_Wmemcella, NCH, 1) +
         bct->transreson (circuit_param_t::cparam_Wmemcella * circuit_param_t::cparam_Wmemcellbscale, NCH, 1);
    c1 = Ccolmux;
    c2 = Cline;

    /*dt: If a wordline segment is shorter than a set, then multiple segments have to be activated */
    if (Ndwl < Nspd) {
        Nact = 1;
    }
    else {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //Nact = Ndwl/Nspd;
        Nact = (int) (Ndwl / Nspd + EPSILON);
    }

    dynRdEnergy += c1 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
    /*
    was: *power += c2 * circuit_param_t::cparam_VddPow * VbitprePow * cols;
    now: we only have limited bitline swing thanks to pulsed wordlines. From looking at papers from industry
    I'm assuming that bitline swing can be limited to Vbitswing 100mV */
    dynRdEnergy += c2 * circuit_param_t::cparam_VddPow * VbitprePow * cols * Nact;

    /*dt: assuming full swing on the bitlines for writes, but only circuit_param_t::cparam_BITOUT bitlines are activated.*/
    dynWrtEnergy += c2 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_BITOUT;

    //fprintf(stderr, "Pow %f %f\n", c1*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_BITOUT*A*circuit_param_t::cparam_muxover*1e9, c2*circuit_param_t::cparam_VddPow*VbitprePow*cols*1e9);
    tstep =
        (r2 * c2 + (r1 + r2) * c1) * log ((Vbitpre) / (Vbitpre - Vbitsense));

    /* take input rise time into account */

    m = Vdd / inrisetime;
    if (tstep <= (0.5 * (Vdd - Vt) / m))
        //v4.1: Using the CACTI journal paper version equation under this condition
    {
        /*a = m;
        b = 2 * ((Vdd * 0.5) - Vt);
        c = -2 * tstep * (Vdd - Vt) + 1 / m * ((Vdd * 0.5) - Vt) *
        ((Vdd * 0.5) - Vt);
        Tbit = (-b + sqrt (b * b - 4 * a * c)) / (2 * a);*/

        Tbit = sqrt(2 * tstep * (Vdd - Vt) / m);
    }
    else
    {
        Tbit = tstep + (Vdd + Vt) / (2 * m) - (Vdd * 0.5) / m;
    }

    *outrisetime = Tbit / (log ((Vbitpre - Vbitsense) / Vdd));


    power->writeOp.dynamic = dynWrtEnergy;
    power->writeOp.leakage = (Icell + Iport) * circuit_param_t::cparam_VddPow;

    power->readOp.dynamic = dynRdEnergy;
    power->readOp.leakage = (Icell + Iport) * circuit_param_t::cparam_VddPow;

    return (Tbit);
}




/*----------------------------------------------------------------------*/

/* Tag array bitline: (see section 6.4 in tech report) */

/*dt: added leakage from eCacti, fixed bitline swing being too large*/
double cacti_time_t::bitline_tag_delay (int C, int A, int B, int Ntwl, int Ntbl, int Ntspd,
                                        double NSubbanks, double inrisetime,
                                        double *outrisetime, powerDef *power, double Tpre)
{
    //double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2, a, b, c;
    double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2;
    double m, tstep, Rpdrive;
    double Cbitrow;		/* bitline capacitance due to access transistor */
    int tagbits;
    int rows, cols, Nact;
    double lkgCell = 0.0;
    double lkgPort = 0.0;
    double dynRdEnergy = 0.0, dynWrtEnergy = 0.0;

    // leakage current of a memory bit-cell

    lkgPort = cmos_ileakage(circuit_param_t::cparam_Wmemcella   , 0           , Vt_cell_nmos_high, Vthn, Vt_cell_pmos_high, Vthp);
    lkgCell = cmos_ileakage(circuit_param_t::cparam_Wmemcellnmos, circuit_param_t::cparam_Wmemcellpmos, Vt_cell_nmos_high, Vthn, Vt_cell_pmos_high, Vthp);

    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) + bct->logtwo ((double) A) -
        //(int) (bct->logtwo (NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) + bct->logtwo ((double) A) -
                         bct->logtwo (NSubbanks) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }

    Cbitrow = bct->draincap(circuit_param_t::cparam_Wmemcella, NCH, 1) / 2.0;	/* due to shared contact */
    rows = C / (B * A * Ntbl * Ntspd);
    cols = tagbits * A * Ntspd / Ntwl;

    // leakage current for the the whole memory core -
    lkgCell *= C / B * tagbits;
    lkgPort *= C / B * tagbits;

    // Determine equivalent width of bitline precharge transistors
    // we assume that Precharge time = decoder delay + wordlineDriver delay time.
    // This is because decoder delay + wordline driver delay contributes to most
    // of the read/write access times.

    Cline = rows * (Cbitrow + circuit_param_t::cparam_Cbitmetal);
    Rpdrive = Tpre / (Cline * log(VSINV) * -1.0);
    circuit_param_t::cparam_Wtbitpreequ = bct->restowidth(Rpdrive, PCH);

    // Note that circuit_param_t::cparam_Wbitpreequ is the equiv. pch transistor width
    circuit_param_t::cparam_Wtpch = 2.0 / 3.0 * circuit_param_t::cparam_Wtbitpreequ;

    if (circuit_param_t::cparam_Wtpch > circuit_param_t::cparam_Wpchmax) {
        circuit_param_t::cparam_Wtpch = circuit_param_t::cparam_Wpchmax;
    }

    // width of sense precharge
    // (depends on width of sense-amp transistors and output driver size)
    // ToDo: calculate the width of sense-amplifier as a function of access times...

    //v4.1: Expressing all widths in terms of circuit_param_t::cparam_FEATURESIZE of input tech node

    circuit_param_t::cparam_WtsPch = 6.25 * circuit_param_t::cparam_FEATURESIZE; // ToDo: Determine widths of these devices analytically; was 5 micron for the 0.8 micron process
    circuit_param_t::cparam_WtsenseN = 3.75 * circuit_param_t::cparam_FEATURESIZE; // sense amplifier N-trans; was 3 micron for the 0.8 micron process
    circuit_param_t::cparam_WtsenseP = 7.5 * circuit_param_t::cparam_FEATURESIZE; // sense amplifier P-trans; was 6 micron for the 0.8 micron process
    circuit_param_t::cparam_WtsenseEn = 5 * circuit_param_t::cparam_FEATURESIZE; // Sense enable transistor of the sense amplifier; was 4 micron for the 0.8 micron process
    circuit_param_t::cparam_WtoBufP = 10 * circuit_param_t::cparam_FEATURESIZE; // output buffer corresponding to the sense amplifier; was 8 micron for the 0.8 micron process
    circuit_param_t::cparam_WtoBufN = 5 * circuit_param_t::cparam_FEATURESIZE; //was 4 micron for the 0.8 micron process
    circuit_param_t::cparam_Wtiso = 12.5 * circuit_param_t::cparam_FEATURESIZE;//was 10 micron for the 0.8 micron process

    //v4.1: Ntspd*Ntbl in the colmux of the else condition does not make sense and overestimates
    //tag bitline delay by a huge amount when Ntspd and Ndbl are large. Fixing this and fixing the if
    //condition as well
    /*if (Ntbl * Ntspd == 1)
      {
        Cline = rows*(Cbitrow+circuit_param_t::cparam_TagCbitmetal)+2*bct->draincap(circuit_param_t::cparam_Wtbitpreequ,PCH,1);
         Ccolmux = bct->gatecap(circuit_param_t::cparam_WtsenseN+circuit_param_t::cparam_WtsenseP,10.0 / circuit_param_t::cparam_FUDGEFACTOR);
         Rlineb = circuit_param_t::cparam_TagRbitmetal*rows/2.0;
         r1 = Rlineb;
      }
    else
      {
        Cline = rows*(Cbitrow+circuit_param_t::cparam_TagCbitmetal) + 2*bct->draincap(circuit_param_t::cparam_Wtbitpreequ,PCH,1) +
                   bct->draincap(circuit_param_t::cparam_Wtiso,PCH,1);
         Ccolmux = Ntspd*Ntbl*(bct->draincap(circuit_param_t::cparam_Wtiso,PCH,1))+bct->gatecap(circuit_param_t::cparam_WtsenseN+circuit_param_t::cparam_WtsenseP,10.0 /circuit_param_t::cparam_FUDGEFACTOR);
         Rlineb = circuit_param_t::cparam_TagRbitmetal*rows/2.0;
       r1 = Rlineb + bct->transreson(circuit_param_t::cparam_Wtiso,PCH,1);
      }*/

    if (Ntspd / Ntwl < 1){ //Ntspd/Ntwl = Tag subarray columns / Tag array output width =
        //= (A * tagbits * Ntspd / Ntwl) / A * tagbits = Degree of column muxing in a tag subarray.
        //When it's less than 1, it means that more than one tag subarray is required to produce the
        //tag array output width.
        Cline = rows * (Cbitrow + circuit_param_t::cparam_TagCbitmetal) + 2 * bct->draincap(circuit_param_t::cparam_Wtbitpreequ, PCH, 1);
        Ccolmux = bct->gatecap(circuit_param_t::cparam_WtsenseN + circuit_param_t::cparam_WtsenseP, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        Rlineb = circuit_param_t::cparam_TagRbitmetal * rows / 2.0;
        r1 = Rlineb;
    }
    else{//Tag array output width comes from one tag subarray
        Cline = rows * (Cbitrow + circuit_param_t::cparam_TagCbitmetal) + 2 * bct->draincap(circuit_param_t::cparam_Wtbitpreequ, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wtiso, PCH, 1);
        Ccolmux = Ntspd / Ntwl * (bct->draincap(circuit_param_t::cparam_Wtiso, PCH, 1)) + bct->gatecap(circuit_param_t::cparam_WtsenseN + circuit_param_t::cparam_WtsenseP, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        Rlineb = circuit_param_t::cparam_TagRbitmetal * rows / 2.0;
        r1 = Rlineb + bct->transreson(circuit_param_t::cparam_Wtiso, PCH, 1);
    }

    r2 = bct->transreson (circuit_param_t::cparam_Wmemcella, NCH, 1) +
         bct->transreson (circuit_param_t::cparam_Wmemcella * circuit_param_t::cparam_Wmemcellbscale, NCH, 1);

    c1 = Ccolmux;
    c2 = Cline;
    //v4.1: The tag energy calculation did not make use of the computed Nact below. Fixing that.


    //dynRdEnergy += c1 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    // Number of active Ntwl blocks
    if (Ntwl / Ntspd < 1) {
        Nact = 1;
    }
    else {
        Nact = Ntwl / Ntspd;
    }

    //dynRdEnergy += c2 * circuit_param_t::cparam_VddPow * VbitprePow * cols;

    dynRdEnergy += c1 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * Nact;

    dynRdEnergy += c2 * circuit_param_t::cparam_VddPow * VbitprePow * cols * Nact;

    if (cols > tagbits)
    {
        dynWrtEnergy += c2 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * tagbits;
        dynWrtEnergy += c2 * circuit_param_t::cparam_VddPow * VbitprePow * (cols - tagbits);
    }
    else {
        dynWrtEnergy += c2 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * tagbits;
    }

    //fprintf(stderr, "Pow %f %f\n", c1*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*1e9, c2*circuit_param_t::cparam_VddPow*VbitprePow*cols*1e9);

    tstep =
        (r2 * c2 + (r1 + r2) * c1) * log ((Vbitpre) / (Vbitpre - Vbitsense));

    /* take into account input rise time */

    m = Vdd / inrisetime;
    if (tstep <= (0.5 * (Vdd - Vt) / m))
        //v4.1: Using the CACTI journal paper version equation under this condition
    {
        /*a = m;
        b = 2 * ((Vdd * 0.5) - Vt);
        c = -2 * tstep * (Vdd - Vt) + 1 / m * ((Vdd * 0.5) - Vt) *
        ((Vdd * 0.5) - Vt);
        Tbit = (-b + sqrt (b * b - 4 * a * c)) / (2 * a);*/

        Tbit = sqrt(2 * tstep * (Vdd - Vt) / m);
    }
    else
    {
        Tbit = tstep + (Vdd + Vt) / (2 * m) - (Vdd * 0.5) / m;
    }

    *outrisetime = Tbit / (log ((Vbitpre - Vbitsense) / Vdd));

    power->readOp.dynamic = dynRdEnergy;
    power->readOp.leakage = (lkgCell + lkgPort) * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = dynWrtEnergy;
    power->writeOp.leakage = (lkgCell + lkgPort) * circuit_param_t::cparam_VddPow;

    return (Tbit);
}



/*----------------------------------------------------------------------*/

/* It is assumed the sense amps have a constant delay
   (see section 6.5) */

double cacti_time_t::sense_amp_delay (int C, int B, int A, int Ndwl, int Ndbl,
                                      double Nspd, double inrisetime, double *outrisetime, powerDef *power)
{
    double Cload;
    int cols, Nact;
    double IsenseEn, IsenseN, IsenseP, IoBufP, IoBufN, Iiso, Ipch, IsPch;
    double lkgIdlePh, lkgReadPh, lkgWritePh;
    double lkgRead, lkgWrite, lkgIdle;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //cols = CHUNKSIZE*B*A*Nspd/Ndwl;
    cols = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);

    // Number of active blocks in Ntwl modules during a read op
    if (Ndwl / Nspd < 1) {
        Nact = 1;
    }
    else {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //Nact = Ndwl/Nspd;
        Nact = (int) (Ndwl / Nspd + EPSILON);
    }

    if (circuit_param_t::cparam_dualVt == 1)
    {
        IsenseEn = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WsenseEn * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IsenseN  = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WsenseN * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IsenseP  = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WsenseP * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        IoBufN   = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WoBufN * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IoBufP   = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WoBufP * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        Iiso     = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_Wiso * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        Ipch     = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_Wpch * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        IsPch    = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WsPch * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
    }
    else {
        IsenseEn = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WsenseEn * circuit_param_t::cparam_inv_Leff, Vthn);
        IsenseN  = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WsenseN * circuit_param_t::cparam_inv_Leff, Vthn);
        IsenseP  = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WsenseP * circuit_param_t::cparam_inv_Leff, Vthp);
        IoBufN   = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WoBufN * circuit_param_t::cparam_inv_Leff, Vthn);
        IoBufP   = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WoBufP * circuit_param_t::cparam_inv_Leff, Vthp);
        Iiso     = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_Wiso * circuit_param_t::cparam_inv_Leff, Vthn);
        Ipch     = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_Wpch * circuit_param_t::cparam_inv_Leff, Vthp);
        IsPch    = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WsPch * circuit_param_t::cparam_inv_Leff, Vthp);
    }

    lkgIdlePh = IsenseEn + 2 * IoBufP;
    lkgWritePh = 2 * Ipch + Iiso + IsenseEn + 2 * IoBufP;
    lkgReadPh = 2 * IsPch + Iiso + IsenseN + IsenseP + IoBufN + IoBufP;

    // read cols in the inactive blocks would be in idle phase
    lkgRead = lkgReadPh * cols * Nact + lkgIdlePh * cols * (Nact - 1);
    // only the cols in which data is written into are in write ph
    // all the remaining cols are considered to be in idle phase
    lkgWrite = lkgWritePh * circuit_param_t::cparam_BITOUT + lkgIdlePh * (cols * Ndwl - circuit_param_t::cparam_BITOUT);
    lkgIdle = lkgIdlePh * cols * Ndwl;

    // sense amplifier has to drive logic in "data out driver" and sense precharge load.
    // load seen by sense amp

    Cload = bct->gatecap(circuit_param_t::cparam_WsenseP + circuit_param_t::cparam_WsenseN, 5.0 / circuit_param_t::cparam_FUDGEFACTOR) + bct->draincap(circuit_param_t::cparam_WsPch, PCH, 1) +
            bct->gatecap(circuit_param_t::cparam_Woutdrvnandn + circuit_param_t::cparam_Woutdrvnandp, 1.0 / circuit_param_t::cparam_FUDGEFACTOR) + bct->gatecap(circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 1.0 / circuit_param_t::cparam_FUDGEFACTOR);

    *outrisetime = circuit_param_t::cparam_tfalldata;
    power->readOp.dynamic = 0.5 * Cload * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;
    power->readOp.leakage = ((lkgRead + lkgIdle) / 2 ) * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = ((lkgWrite + lkgIdle) / 2 ) * circuit_param_t::cparam_VddPow;

    return(circuit_param_t::cparam_tsensedata + 2.5e-10 / circuit_param_t::cparam_FUDGEFACTOR); //v4.1: Computing sense amp delay component for input tech node itself instead
    //of dividing by circuit_param_t::cparam_FUDGEFACTOR at the end.
}

/*--------------------------------------------------------------*/

double cacti_time_t::sense_amp_tag_delay (int C, int B, int A, int Ntwl, int Ntbl,
        int Ntspd, double NSubbanks, double inrisetime, double *outrisetime, powerDef *power)
{
    double Cload;
    int cols, tagbits, Nact;
    double IsenseEn, IsenseN, IsenseP, IoBufP, IoBufN, Iiso, Ipch, IsPch;
    double lkgIdlePh, lkgReadPh, lkgWritePh;
    double lkgRead, lkgWrite, lkgIdle;

    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS -bct->logtwo((double)C)+bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - (int)(bct->logtwo(NSubbanks)) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    cols = tagbits * A * Ntspd / Ntwl;

    // Number of active blocks in Ntwl modules during a read op
    if (Ntwl / Ntspd < 1) {
        Nact = 1;
    }
    else {
        Nact = Ntwl / Ntspd;
    }

    if (circuit_param_t::cparam_dualVt == 1)
    {
        IsenseEn = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtsenseEn * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IsenseN = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtsenseN * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IsenseP = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtsenseP * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        IoBufN = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtoBufN * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        IoBufP = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtoBufP * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        Iiso = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_Wtiso * circuit_param_t::cparam_inv_Leff, Vt_cell_nmos_high);
        Ipch = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_Wtpch * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
        IsPch = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtsPch * circuit_param_t::cparam_inv_Leff, Vt_cell_pmos_high);
    }
    else
    {
        IsenseEn = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtsenseEn * circuit_param_t::cparam_inv_Leff, Vthn);
        IsenseN = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtsenseN * circuit_param_t::cparam_inv_Leff, Vthn);
        IsenseP = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtsenseP * circuit_param_t::cparam_inv_Leff, Vthp);
        IoBufN = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_WtoBufN * circuit_param_t::cparam_inv_Leff, Vthn);
        IoBufP = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtoBufP * circuit_param_t::cparam_inv_Leff, Vthp);
        Iiso = leakage_obj->simplified_nmos_leakage(circuit_param_t::cparam_Wtiso * circuit_param_t::cparam_inv_Leff, Vthn);
        Ipch = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_Wtpch * circuit_param_t::cparam_inv_Leff, Vthp);
        IsPch = leakage_obj->simplified_pmos_leakage(circuit_param_t::cparam_WtsPch * circuit_param_t::cparam_inv_Leff, Vthp);
    }

    lkgIdlePh = IsenseEn + 2 * IoBufP;
    lkgWritePh = 2 * Ipch + Iiso + IsenseEn + 2 * IoBufP;
    lkgReadPh = 2 * IsPch + Iiso + IsenseN + IsenseP + IoBufN + IoBufP;

    // read cols in the inactive blocks would be in idle phase
    lkgRead = lkgReadPh * cols * Nact + lkgIdlePh * cols * (Nact - 1);
    // only the cols in which data is written into are in write ph
    // all the remaining cols are considered to be in idle phase
    lkgWrite = lkgWritePh * tagbits + lkgIdlePh * (cols * Ntwl - tagbits);
    lkgIdle = lkgIdlePh * cols * Ntwl;

    // sense amplifier has to drive logic in "data out driver" and sense precharge load.
    // load seen by sense amp

    Cload = bct->gatecap(circuit_param_t::cparam_WtsenseP + circuit_param_t::cparam_WtsenseN, 5.0 / circuit_param_t::cparam_FUDGEFACTOR) + bct->draincap(circuit_param_t::cparam_WtsPch, PCH, 1);
    //bct->gatecap(circuit_param_t::cparam_Woutdrvnandn+circuit_param_t::cparam_Woutdrvnandp,1.0) + bct->gatecap(circuit_param_t::cparam_Woutdrvnorn+circuit_param_t::cparam_Woutdrvnorp,1.0);

    *outrisetime = circuit_param_t::cparam_tfalltag;
    power->readOp.dynamic = 0.5 * Cload * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * tagbits * A;
    power->readOp.leakage = ((lkgRead + lkgIdle) / 2 ) * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = ((lkgWrite + lkgIdle) / 2 ) * circuit_param_t::cparam_VddPow;

    return(circuit_param_t::cparam_tsensetag + 2.5e-10 / circuit_param_t::cparam_FUDGEFACTOR); //v4.1: Computing sense amp delay component for input tech node itself instead
    //of dividing by circuit_param_t::cparam_FUDGEFACTOR at the end.
}

/*----------------------------------------------------------------------*/

/* Comparator Delay (see section 6.6) */


double cacti_time_t::compare_time (int C, int A, int Ntbl, int Ntspd,
                                   double NSubbanks, double inputtime, double *outputtime, powerDef *power)
{
    double Req, Ceq, tf, st1del, st2del, st3del, nextinputtime, m;
    double c1, c2, r1, r2, tstep, a, b, c;
    double Tcomparatorni;
    int cols, tagbits;
    double lkgCurrent = 0.0, dynPower = 0.0;

    /* First Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wcompinvn2 + circuit_param_t::cparam_Wcompinvp2, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp1, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn1, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvp1, PCH, 1);
    tf = Req * Ceq;
    st1del = bct->horowitz(inputtime, tf, VTHCOMPINV, VTHCOMPINV, FALL);
    nextinputtime = st1del / VTHCOMPINV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent = 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn1, circuit_param_t::cparam_Wcompinvp1, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    /* Second Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wcompinvn3 + circuit_param_t::cparam_Wcompinvp3, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp2, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn2, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvn2, NCH, 1);
    tf = Req * Ceq;
    st2del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHCOMPINV, RISE);
    nextinputtime = st2del / (1.0 - VTHCOMPINV);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn2, circuit_param_t::cparam_Wcompinvp2, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /* Third Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wevalinvn + circuit_param_t::cparam_Wevalinvp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp3, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn3, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvp3, PCH, 1);
    tf = Req * Ceq;
    st3del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHEVALINV, FALL);
    nextinputtime = st3del / (VTHEVALINV);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn3, circuit_param_t::cparam_Wcompinvp3, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /* Final Inverter (virtual ground driver) discharging compare part */

    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - (int)(bct->logtwo(NSubbanks)) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    tagbits = tagbits / 2;
    cols = tagbits * Ntbl * Ntspd;

    r1 = bct->transreson(circuit_param_t::cparam_Wcompn, NCH, 2);
    r2 = bct->transreson(circuit_param_t::cparam_Wevalinvn, NCH, 1); /* was switch */
    c2 = (tagbits) * (bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 2)) +
         bct->draincap(circuit_param_t::cparam_Wevalinvp, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wevalinvn, NCH, 1);
    c1 = (tagbits) * (bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 2))
         + bct->draincap(circuit_param_t::cparam_Wcompp, PCH, 1) + bct->gatecap(circuit_param_t::cparam_WmuxdrvNANDn + circuit_param_t::cparam_WmuxdrvNANDp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
         cols * circuit_param_t::cparam_Cwordmetal;
    dynPower += c2 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;
    dynPower += c1 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * (A - 1);

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wevalinvn, circuit_param_t::cparam_Wevalinvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    // double the tag_bits as the delay is being computed for only
    // half the number. The leakage is still for the total tag bits

    // the comparator compares cols number of bits. Since these transistors are
    // stacked, a stacking factor of 0.2 is used
    lkgCurrent += 2 * cols * 0.2 * 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompn, circuit_param_t::cparam_Wcompp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    // leakage due to the mux driver
    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wmuxdrv12n, circuit_param_t::cparam_Wmuxdrv12p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /* time to go to threshold of mux driver */

    tstep = (r2 * c2 + (r1 + r2) * c1) * log(1.0 / VTHMUXNAND);

    /* take into account non-zero input rise time */

    m = Vdd / nextinputtime;

    if ((tstep) <= (0.5*(Vdd - Vt) / m)) {
        a = m;
        b = 2 * ((Vdd * VTHEVALINV) - Vt);
        c = -2 * (tstep) * (Vdd - Vt) + 1 / m * ((Vdd * VTHEVALINV) - Vt) * ((Vdd * VTHEVALINV) - Vt);
        Tcomparatorni = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
    } else {
        Tcomparatorni = (tstep) + (Vdd + Vt) / (2 * m) - (Vdd * VTHEVALINV) / m;
    }
    *outputtime = Tcomparatorni / (1.0 - VTHMUXNAND);

    power->readOp.dynamic = dynPower;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    return(Tcomparatorni + st1del + st2del + st3del);
}




/*----------------------------------------------------------------------*/

/* Delay of the multiplexor Driver (see section 6.7) */


double cacti_time_t::mux_driver_delay (int Ntbl, int Ntspd, double inputtime,
                                       double *outputtime, double wirelength)
{
    double Ceq, Req, tf, nextinputtime;
    double Tst1, Tst2, Tst3;

    /* first driver stage - Inverte "match" to produce "matchb" */
    /* the critical path is the DESELECTED case, so consider what
       happens when the address bit is true, but match goes low */

    Ceq = bct->gatecap (circuit_param_t::cparam_WmuxdrvNORn + circuit_param_t::cparam_WmuxdrvNORp, 15.0 / circuit_param_t::cparam_FUDGEFACTOR) * circuit_param_t::cparam_muxover +
          bct->draincap(circuit_param_t::cparam_Wmuxdrv12n, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wmuxdrv12p, PCH, 1);
    Req = bct->transreson (circuit_param_t::cparam_Wmuxdrv12p, PCH, 1);
    tf = Ceq * Req;
    Tst1 = bct->horowitz (inputtime, tf, VTHMUXDRV1, VTHMUXDRV2, FALL);
    nextinputtime = Tst1 / VTHMUXDRV2;

    /* second driver stage - NOR "matchb" with address bits to produce sel */

    Ceq =
        bct->gatecap (circuit_param_t::cparam_Wmuxdrv3n + circuit_param_t::cparam_Wmuxdrv3p, 15.0 / circuit_param_t::cparam_FUDGEFACTOR) + 2 * bct->draincap(circuit_param_t::cparam_WmuxdrvNORn, NCH,
                1) +
        bct->draincap(circuit_param_t::cparam_WmuxdrvNORp, PCH, 2);
    Req = bct->transreson (circuit_param_t::cparam_WmuxdrvNORn, NCH, 1);
    tf = Ceq * Req;
    Tst2 = bct->horowitz (nextinputtime, tf, VTHMUXDRV2, VTHMUXDRV3, RISE);
    nextinputtime = Tst2 / (1 - VTHMUXDRV3);

    /* third driver stage - invert "select" to produce "select bar" */


    Ceq =
        circuit_param_t::cparam_BITOUT * bct->gatecap (circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0)
        + bct->draincap(circuit_param_t::cparam_Wmuxdrv3p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wmuxdrv3n, NCH, 1) +
        circuit_param_t::cparam_GlobalCwordmetal * wirelength;
    Req = (circuit_param_t::cparam_GlobalRwordmetal * wirelength) / 2.0 + bct->transreson (circuit_param_t::cparam_Wmuxdrv3p, PCH, 1);
    tf = Ceq * Req;
    Tst3 = bct->horowitz (nextinputtime, tf, VTHMUXDRV3, VTHOUTDRINV, FALL);
    *outputtime = Tst3 / (VTHOUTDRINV);

    return (Tst1 + Tst2 + Tst3);

}


void cacti_time_t::precalc_muxdrv_widths(int C, int B, int A, int Ndwl, int Ndbl, double Nspd,
        double * wirelength_v, double * wirelength_h)
{
    int l_muxdrv_v = 0, l_muxdrv_h = 0, cols_subarray, rows_subarray, nr_subarrays_left, horizontal_step, vertical_step, v_or_h;
    //double wirelength, Ceq, Cload, Cline;
    double Ceq, Cload, Cline;
    int wire_v = 0, wire_h = 0;
    double current_ndriveW, current_pdriveW, previous_ndriveW, previous_pdriveW;

    //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
    //the final int value is the correct one
    //cols_subarray = (CHUNKSIZE * B * A * Nspd / Ndwl);
    //rows_subarray = (C /(B * A * Ndbl * Nspd));
    cols_subarray = (int) (CHUNKSIZE * B * A * Nspd / Ndwl + EPSILON);
    rows_subarray = (int) (C / (B * A * Ndbl * Nspd) + EPSILON);

    if (Ndwl * Ndbl == 1) {
        l_muxdrv_v = 0;
        l_muxdrv_h = cols_subarray;

        Cmuxdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * cols_subarray;
        Rmuxdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * cols_subarray;

        Cline = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
                circuit_param_t::cparam_GlobalCwordmetal * cols_subarray;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        nr_muxdrvtreesegments = 0;

    }
    else if (Ndwl * Ndbl == 2
             || Ndwl * Ndbl == 4) {
        l_muxdrv_v = 0;
        l_muxdrv_h = 2 * cols_subarray;

        Cmuxdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * cols_subarray;
        Rmuxdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * cols_subarray;

        Cline = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
                circuit_param_t::cparam_GlobalCwordmetal * cols_subarray;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        nr_muxdrvtreesegments = 0;
    }
    else if (Ndwl * Ndbl > 4) {
        nr_subarrays_left = Ndwl * Ndbl;
        nr_subarrays_left /= 2;
        horizontal_step = cols_subarray;
        vertical_step = rows_subarray;
        l_muxdrv_h = horizontal_step;

        Cmuxdrvtreesegments[0] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        Rmuxdrvtreesegments[0] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;

        Cline = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
                circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
        Cload = Cline / bct->gatecap(1.0, 0.0);

        current_ndriveW = Cload * SizingRatio / 3;
        current_pdriveW = 2 * Cload * SizingRatio / 3;

        WmuxdrvtreeN[0] = current_ndriveW;
        nr_muxdrvtreesegments = 1;

        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            previous_ndriveW = current_ndriveW;
            previous_pdriveW = current_pdriveW;
            nr_muxdrvtreesegments++;
            if (v_or_h) {
                l_muxdrv_v += vertical_step;
                v_or_h = 0;

                Cmuxdrvtreesegments[nr_muxdrvtreesegments-1] = circuit_param_t::cparam_GlobalCbitmetal * vertical_step;
                Rmuxdrvtreesegments[nr_muxdrvtreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRbitmetal * vertical_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCbitmetal * vertical_step;

                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                l_muxdrv_h += horizontal_step;
                v_or_h = 1;

                Cmuxdrvtreesegments[nr_muxdrvtreesegments-1] = circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;
                Rmuxdrvtreesegments[nr_muxdrvtreesegments-1] = 0.5 * circuit_param_t::cparam_GlobalRwordmetal * horizontal_step;
                Cline = bct->gatecap(previous_ndriveW + previous_pdriveW, 0) + circuit_param_t::cparam_GlobalCwordmetal * horizontal_step;

                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }

            Cload = Cline / bct->gatecap(1.0, 0.0);

            current_ndriveW = Cload * SizingRatio / 3;
            current_pdriveW = 2 * Cload * SizingRatio / 3;

            WmuxdrvtreeN[nr_muxdrvtreesegments-1] = current_ndriveW;
        }
    }

    wire_v = (l_muxdrv_v);
    wire_h = (l_muxdrv_h);



    Ceq = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR);
    /*dt: adding an extra layer of drivers, so  that we don't have these biiiig gates */
    Ceq += circuit_param_t::cparam_GlobalCwordmetal * wire_h + circuit_param_t::cparam_GlobalCbitmetal * wire_v;
    Cload = Ceq / bct->gatecap(1.0, 0.0);
    circuit_param_t::cparam_Wmuxdrv3p = Cload * SizingRatio * 2 / 3;
    circuit_param_t::cparam_Wmuxdrv3n = Cload * SizingRatio / 3;


    circuit_param_t::cparam_WmuxdrvNORn = (circuit_param_t::cparam_Wmuxdrv3n + circuit_param_t::cparam_Wmuxdrv3p) * SizingRatio / 3;

    // 2 to account for series pmos in NOR
    circuit_param_t::cparam_WmuxdrvNORp = 2 * (circuit_param_t::cparam_Wmuxdrv3n + circuit_param_t::cparam_Wmuxdrv3p) * SizingRatio * 2 / 3;

    circuit_param_t::cparam_WmuxdrvNANDn = 2 * (circuit_param_t::cparam_WmuxdrvNORn + circuit_param_t::cparam_WmuxdrvNORp) * circuit_param_t::cparam_muxover * SizingRatio * 1 / 3;
    circuit_param_t::cparam_WmuxdrvNANDp = (circuit_param_t::cparam_WmuxdrvNORn + circuit_param_t::cparam_WmuxdrvNORp) * circuit_param_t::cparam_muxover * SizingRatio * 2 / 3;

    *wirelength_h = wire_h;
    *wirelength_v = wire_v;

}
/*dt: incorporated leakage and resizing from eCacti*/


double cacti_time_t::mux_driver_delay_dualin (int C, int B, int A, int Ntbl, int Ntspd, double inputtime1,
        double *outputtime, double wirelength_v, double wirelength_h, powerDef *power)
{
    //double Ceq,Req,Rwire,tf,nextinputtime, Cload;
    double Ceq, Req, Rwire, tf, nextinputtime;
    //double Tst1,Tst2,Tst2a,Tst3,Tst4;
    double Tst1, Tst2, Tst3;
    double lkgCurrent = 0.0, dynPower = 0.0;
    //double Wwiredrv1n, Wwiredrv1p, Wmuxdrv4n, Wmuxdrv4p;
    double overall_delay = 0;
    int i = 0;
    double inrisetime = 0, this_delay = 0;

    /* first driver stage - Inverte "match" to produce "matchb" */
    /* the critical path is the DESELECTED case, so consider what
       happens when the address bit is true, but match goes low */

    Ceq = bct->gatecap(circuit_param_t::cparam_WmuxdrvNORn + circuit_param_t::cparam_WmuxdrvNORp, 15.0 / circuit_param_t::cparam_FUDGEFACTOR) * circuit_param_t::cparam_muxover
          + bct->draincap(circuit_param_t::cparam_WmuxdrvNANDn, NCH, 2) + 2 * bct->draincap(circuit_param_t::cparam_WmuxdrvNANDp, PCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_WmuxdrvNANDp, PCH, 1);
    tf = Ceq * Req;
    Tst1 = bct->horowitz(inputtime1, tf, VTHMUXNAND, VTHMUXDRV2, FALL);
    nextinputtime = Tst1 / VTHMUXDRV2;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * (A - 1);

    lkgCurrent = 0.5 * 0.2 * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_WmuxdrvNORn, circuit_param_t::cparam_WmuxdrvNORp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    lkgCurrent += 0.5 * 0.2 * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_WmuxdrvNANDn, circuit_param_t::cparam_WmuxdrvNANDp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);


    /* second driver stage - NOR "matchb" with address bits to produce sel */
    Ceq = bct->gatecap(circuit_param_t::cparam_Wmuxdrv3n + circuit_param_t::cparam_Wmuxdrv3p, 15.0 / circuit_param_t::cparam_FUDGEFACTOR) + 2 * bct->draincap(circuit_param_t::cparam_WmuxdrvNORn, NCH, 1) +
          bct->draincap(circuit_param_t::cparam_WmuxdrvNORp, PCH, 2);
    Req = bct->transreson(circuit_param_t::cparam_WmuxdrvNORn, NCH, 1);
    tf = Ceq * Req;
    Tst2 = bct->horowitz(nextinputtime, tf, VTHMUXDRV2, VTHMUXDRV3, RISE);
    nextinputtime = Tst2 / (1 - VTHMUXDRV3);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent += 0.5 *  circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Wmuxdrv3n, circuit_param_t::cparam_Wmuxdrv3p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /*dt: doing all the H-tree segments*/
    inrisetime = nextinputtime;
    Tst3 = 0;

    if (nr_muxdrvtreesegments) {
        Ceq = bct->draincap(circuit_param_t::cparam_Wmuxdrv3p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wmuxdrv3n, NCH, 1) +
              bct->gatecap(3 * WmuxdrvtreeN[nr_muxdrvtreesegments-1], 0) + Cmuxdrvtreesegments[nr_muxdrvtreesegments-1];
        Req = bct->transreson(circuit_param_t::cparam_Wmuxdrv3n, NCH, 1) + Rmuxdrvtreesegments[nr_muxdrvtreesegments-1];

        tf = Ceq * Req;
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);

        Tst3 += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += A * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += A * 0.5 * cmos_ileakage(circuit_param_t::cparam_Wmuxdrv3n, circuit_param_t::cparam_Wmuxdrv3p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }

    for (i = nr_muxdrvtreesegments; i > 2;i--) {
        /*dt: this too should alternate...*/
        Ceq = (Cmuxdrvtreesegments[i-2] + bct->draincap(2 * WmuxdrvtreeN[i-1], PCH, 1) + bct->draincap(WmuxdrvtreeN[i-1], NCH, 1) +
               bct->gatecap(3 * WmuxdrvtreeN[i-2], 0.0));
        Req = (Rmuxdrvtreesegments[i-2] + bct->transreson(WmuxdrvtreeN[i-1], NCH, 1));
        tf = Req * Ceq;
        /*dt: This shouldn't be all falling, but interleaved. Have to fix that at some point.*/
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);
        Tst3 += this_delay;
        inrisetime = this_delay / (1.0 - VTHINV360x240);

        dynPower += A * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += pow(2.00, (double) circuit_param_t::cparam_nr_dectreesegments - i) * A * 0.5 *
                      cmos_ileakage(WmuxdrvtreeN[i-1], 2 * WmuxdrvtreeN[i-1], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }

    if (nr_muxdrvtreesegments) {
        Ceq = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
              + circuit_param_t::cparam_Cdectreesegments[0] + bct->draincap(2 * WmuxdrvtreeN[0], PCH, 1) + bct->draincap(WmuxdrvtreeN[0], NCH, 1);
        Rwire = Rmuxdrvtreesegments[0];
        tf = (Rwire + bct->transreson(2 * WmuxdrvtreeN[0], PCH, 1)) * Ceq;
        this_delay = bct->horowitz(inrisetime, tf, VTHINV360x240, VTHOUTDRINV, FALL);;
        Tst3 += this_delay;
        inrisetime = this_delay / VTHOUTDRINV;
        dynPower += A * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += pow(2.00, (double)nr_muxdrvtreesegments) * A * 0.5 * cmos_ileakage(WmuxdrvtreeN[0], 2 * WmuxdrvtreeN[0], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    }
    else {
        Ceq  = circuit_param_t::cparam_BITOUT * bct->gatecap(circuit_param_t::cparam_Woutdrvseln + circuit_param_t::cparam_Woutdrvselp + circuit_param_t::cparam_Woutdrvnorn + circuit_param_t::cparam_Woutdrvnorp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR);
        Ceq += bct->draincap(circuit_param_t::cparam_Wmuxdrv3p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wmuxdrv3n, NCH, 1);
        Ceq += circuit_param_t::cparam_GlobalCwordmetal * wirelength_h + circuit_param_t::cparam_GlobalCbitmetal * wirelength_v;

        Req  = (circuit_param_t::cparam_GlobalRwordmetal * wirelength_h + circuit_param_t::cparam_GlobalRbitmetal * wirelength_v) / 2.0;
        Req += bct->transreson(circuit_param_t::cparam_Wmuxdrv3p, PCH, 1);
        tf = Ceq * Req;
        Tst3 = bct->horowitz(inrisetime, tf, VTHMUXDRV3, VTHOUTDRINV, FALL);
        //nextinputtime = Tst3/VSINV;
        *outputtime = Tst3 / (VTHOUTDRINV);
        dynPower += A * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
    }

    power->readOp.dynamic = dynPower;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;


    overall_delay = (Tst1 + Tst2 + Tst3);
    return overall_delay;
}

double cacti_time_t::senseext_driver_delay(int A, char fullyassoc,
        double inputtime, double *outputtime, double wirelength_sense_v, double wirelength_sense_h, powerDef *power)
{
    double Ceq, Req, tf, nextinputtime;
    //double Tst1,Tst2,Tst3;
    double Tst1, Tst2;
    double lkgCurrent = 0.0, dynPower = 0.0;

    if (fullyassoc) {
        A = 1;
    }

    /* first driver stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Wsenseextdrv1p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv1n, NCH, 1) +
          bct->gatecap(circuit_param_t::cparam_Wsenseextdrv2n + circuit_param_t::cparam_Wsenseextdrv2p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    Req = bct->transreson(circuit_param_t::cparam_Wsenseextdrv1n, NCH, 1);
    tf = Ceq * Req;
    Tst1 = bct->horowitz(inputtime, tf, VTHSENSEEXTDRV, VTHSENSEEXTDRV, FALL);
    nextinputtime = Tst1 / VTHSENSEEXTDRV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;

    lkgCurrent = 0.5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Wsenseextdrv1n, circuit_param_t::cparam_Wsenseextdrv1p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    /* second driver stage */

    Ceq = bct->draincap(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wsenseextdrv2n, NCH, 1) +
          circuit_param_t::cparam_GlobalCwordmetal * wirelength_sense_h + circuit_param_t::cparam_GlobalCbitmetal * wirelength_sense_v;

    Req = (circuit_param_t::cparam_GlobalRwordmetal * wirelength_sense_h + circuit_param_t::cparam_GlobalRbitmetal * wirelength_sense_v) / 2.0 + bct->transreson(circuit_param_t::cparam_Wsenseextdrv2p, PCH, 1);

    tf = Ceq * Req;
    Tst2 = bct->horowitz(nextinputtime, tf, VTHSENSEEXTDRV, VTHOUTDRNAND, RISE);

    *outputtime = Tst2 / (1 - VTHOUTDRNAND);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover;

    lkgCurrent = 0.5 * circuit_param_t::cparam_BITOUT * A * circuit_param_t::cparam_muxover  * cmos_ileakage(circuit_param_t::cparam_Wsenseextdrv2n, circuit_param_t::cparam_Wsenseextdrv2p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    //   fprintf(stderr, "Pow %f %f\n", Ceq*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*.5*circuit_param_t::cparam_BITOUT*A*circuit_param_t::cparam_muxover*1e3, Ceq*circuit_param_t::cparam_VddPow*circuit_param_t::cparam_VddPow*.5*circuit_param_t::cparam_BITOUT*A*circuit_param_t::cparam_muxover*1e3);

    power->readOp.dynamic = dynPower;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    return(Tst1 + Tst2);

}



double cacti_time_t::total_out_driver_delay (int C, int B, int A, char fullyassoc, int Ndbl, double Nspd, int Ndwl, int Ntbl, int Ntwl,
        int Ntspd, double NSubbanks, double inputtime, double *outputtime, powerDef *power)
{
    powerDef single_power;
    double total_senseext_driver, single_senseext_driver;
    int cols_data_subarray, rows_data_subarray, cols_tag_subarray,
    rows_tag_subarray;
    double subbank_v, subbank_h, sub_v, sub_h, inter_v, inter_h, htree;
    int htree_int, tagbits;
    int cols_fa_subarray, rows_fa_subarray;

    reset_powerDef(&single_power);
    total_senseext_driver = 0.0;
    single_senseext_driver = 0.0;

    if (!fullyassoc)
    {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //cols_data_subarray = (8 * B * A * Nspd / Ndwl);
        //rows_data_subarray = (C / (B * A * Ndbl * Nspd));
        cols_data_subarray = (int)(8 * B * A * Nspd / Ndwl + EPSILON);
        rows_data_subarray = (int)(C / (B * A * Ndbl * Nspd) + EPSILON);

        if (Ndwl * Ndbl == 1)
        {
            sub_v = rows_data_subarray;
            sub_h = cols_data_subarray;
        }
        if (Ndwl * Ndbl == 2)
        {
            sub_v = rows_data_subarray;
            sub_h = 2 * cols_data_subarray;
        }

        if (Ndwl * Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndwl * Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl)) * rows_data_subarray;
                sub_h = sqrt ((double) (Ndwl * Ndbl)) * cols_data_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ndwl * Ndbl / 2)) * rows_data_subarray;
                sub_h = 2 * sqrt ((double) (Ndwl * Ndbl / 2)) * cols_data_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        rows_tag_subarray = C / (B * A * Ntbl * Ntspd);

        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
            //(	int) bct->logtwo ((double) A) - (int) (bct->logtwo (NSubbanks));
            tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) +
                             bct->logtwo ((double) A) - (int) (bct->logtwo (NSubbanks)) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_tag_subarray = tagbits * A * Ntspd / Ntwl;

        if (Ntwl * Ntbl == 1)
        {
            sub_v = rows_tag_subarray;
            sub_h = cols_tag_subarray;
        }
        if (Ntwl * Ntbl == 2)
        {
            sub_v = rows_tag_subarray;
            sub_h = 2 * cols_tag_subarray;
        }

        if (Ntwl * Ntbl > 2)
        {
            htree = bct->logtwo ((double) (Ntwl * Ntbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl)) * rows_tag_subarray;
                sub_h = sqrt ((double) (Ntwl * Ntbl)) * cols_tag_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ntwl * Ntbl / 2)) * rows_tag_subarray;
                sub_h = 2 * sqrt ((double) (Ntwl * Ntbl / 2)) * cols_tag_subarray;
            }
        }

        inter_v = MAX (sub_v, inter_v);
        inter_h += sub_h;
        subbank_h = inter_h;
        subbank_v = inter_v;
    }
    else
    {
        rows_fa_subarray = (C / (B * Ndbl));
        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) B);
            tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) B) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_tag_subarray = tagbits;
        cols_fa_subarray = (8 * B) + cols_tag_subarray;

        if (Ndbl == 1)
        {
            sub_v = rows_fa_subarray;
            sub_h = cols_fa_subarray;
        }
        if (Ndbl == 2)
        {
            sub_v = rows_fa_subarray;
            sub_h = 2 * cols_fa_subarray;
        }

        if (Ndbl > 2)
        {
            htree = bct->logtwo ((double) (Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0)
            {
                sub_v = sqrt ((double) Ndbl) * rows_fa_subarray;
                sub_h = sqrt ((double) Ndbl) * cols_fa_subarray;
            }
            else
            {
                sub_v = sqrt ((double) (Ndbl / 2)) * rows_fa_subarray;
                sub_h = 2 * sqrt ((double) (Ndbl / 2)) * cols_fa_subarray;
            }
        }
        inter_v = sub_v;
        inter_h = sub_h;

        subbank_v = inter_v;
        subbank_h = inter_h;
    }


    if (NSubbanks == 1.0 || NSubbanks == 2.0)
    {
        subbank_h = 0;
        subbank_v = 0;
        single_senseext_driver =
            senseext_driver_delay (A, fullyassoc, inputtime, outputtime,
                                   subbank_v, subbank_h, &single_power);
        total_senseext_driver += single_senseext_driver;
        add_powerDef(power, single_power, *power);
    }

    while (NSubbanks > 2.0)
    {
        if (NSubbanks == 4.0)
        {
            subbank_h = 0;
        }

        single_senseext_driver =
            senseext_driver_delay (A, fullyassoc, inputtime, outputtime, subbank_v,
                                   subbank_h, &single_power);

        NSubbanks = NSubbanks / 4.0;
        subbank_v = 2 * subbank_v;
        subbank_h = 2 * subbank_h;
        inputtime = *outputtime;
        total_senseext_driver += single_senseext_driver;
        add_powerDef(power, single_power, *power);
    }
    return (total_senseext_driver);

}

/* Valid driver (see section 6.9 of tech report)
   Note that this will only be called for a direct mapped cache */

double cacti_time_t::valid_driver_delay (int C, int B, int A, char fullyassoc, int Ndbl, int Ndwl,
        double Nspd, int Ntbl, int Ntwl, int Ntspd, double *NSubbanks, double inputtime, powerDef *power)
{
    double Ceq, Tst1, tf;
    int rows, tagbits, cols, l_valdrv_v = 0, l_valdrv_h = 0;
    double wire_cap, wire_res;
    double subbank_v, subbank_h;
    int nr_subarrays_left = 0, v_or_h = 0;
    int horizontal_step = 0, vertical_step = 0;

    rows = C / (8 * B * A * Ntbl * Ntspd);

    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) + bct->logtwo ((double) A) -
        //(int) (bct->logtwo (*NSubbanks));
        tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo ((double) C) + bct->logtwo ((double) A) -
                         (int) (bct->logtwo (*NSubbanks)) + EPSILON);
    }
    else {
        tagbits = force_tag_size;
    }
    cols = tagbits * A * Ntspd / Ntwl;

    /* calculate some layout info */

    if (Ntwl * Ntbl == 1)
    {
        l_valdrv_v = 0;
        l_valdrv_h = cols;
    }
    if (Ntwl * Ntbl == 2 || Ntwl * Ntbl == 4)
    {
        l_valdrv_v = 0;
        l_valdrv_h = cols;
    }
    else {
        nr_subarrays_left = Ntwl * Ntbl;
        nr_subarrays_left /= 4;
        horizontal_step = cols;
        vertical_step = C / (B * A * Ntbl * Ntspd);
        l_valdrv_h = horizontal_step;
        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            if (v_or_h) {
                l_valdrv_v += vertical_step;
                v_or_h = 0;
                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                l_valdrv_h += horizontal_step;
                v_or_h = 1;
                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }
        }
    }

    subbank_routing_length (C, B, A, fullyassoc, Ndbl, Nspd, Ndwl, Ntbl, Ntwl,
                            Ntspd, *NSubbanks, &subbank_v, &subbank_h);

    wire_cap =
        circuit_param_t::cparam_GlobalCbitmetal * (l_valdrv_v + subbank_v) + circuit_param_t::cparam_GlobalCwordmetal * (l_valdrv_h +
                subbank_h);
    wire_res =
        circuit_param_t::cparam_GlobalRwordmetal * (l_valdrv_h + subbank_h) * 0.5 + circuit_param_t::cparam_GlobalRbitmetal * (l_valdrv_v +
                subbank_v) * 0.5;

    Ceq =
        bct->draincap(circuit_param_t::cparam_Wmuxdrv12n, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wmuxdrv12p, PCH,
                1) + wire_cap + circuit_param_t::cparam_Cout;
    tf = Ceq * (bct->transreson (circuit_param_t::cparam_Wmuxdrv12p, PCH, 1) + wire_res);
    Tst1 = bct->horowitz (inputtime, tf, VTHMUXDRV1, 0.5, FALL);
    power->readOp.dynamic += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    return (Tst1);
}

double cacti_time_t::half_compare_delay(int C, int B, int A, int Ntwl, int Ntbl, int Ntspd, double NSubbanks, double inputtime, double *outputtime, powerDef *power)
{
    double Req, Ceq, tf, st1del, st2del, st3del, nextinputtime, m;
    double c1, c2, r1, r2, tstep, a, b, c;
    double Tcomparatorni;
    int cols, tagbits;
    double lkgCurrent = 0.0, dynPower = 0.0;
    int v_or_h = 0, hori = 0, vert = 0,
                                     horizontal_step = 0, vertical_step = 0 , nr_subarrays_left = 0;
    double Wxtramuxdrv1n = 0, Wxtramuxdrv1p = 0, Wxtramuxdrv2n = 0, Wxtramuxdrv2p = 0;
    double Cload = 0;
    double muxdrv1del = 0, muxdrv2del = 0;

    /* First Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wcompinvn2 + circuit_param_t::cparam_Wcompinvp2, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp1, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn1, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvp1, PCH, 1);
    tf = Req * Ceq;
    st1del = bct->horowitz(inputtime, tf, VTHCOMPINV, VTHCOMPINV, FALL);
    nextinputtime = st1del / VTHCOMPINV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent = 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn1, circuit_param_t::cparam_Wcompinvp1, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    /* Second Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wcompinvn3 + circuit_param_t::cparam_Wcompinvp3, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp2, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn2, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvn2, NCH, 1);
    tf = Req * Ceq;
    st2del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHCOMPINV, RISE);
    nextinputtime = st2del / (1.0 - VTHCOMPINV);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn2, circuit_param_t::cparam_Wcompinvp2, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /* Third Inverter */

    Ceq = bct->gatecap(circuit_param_t::cparam_Wevalinvn + circuit_param_t::cparam_Wevalinvp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          bct->draincap(circuit_param_t::cparam_Wcompinvp3, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompinvn3, NCH, 1);
    Req = bct->transreson(circuit_param_t::cparam_Wcompinvp3, PCH, 1);
    tf = Req * Ceq;
    st3del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHEVALINV, FALL);
    nextinputtime = st3del / (VTHEVALINV);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompinvn3, circuit_param_t::cparam_Wcompinvp3, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);


    /* Final Inverter (virtual ground driver) discharging compare part */

    /*dt: Certainly don't need the dirty bit in comparison. The valid bit is assumed to be handled elsewhere, because we don't need to route any data to compare with to it.*/
    if (!force_tag) {
        //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
        //the final int value is the correct one
        //tagbits = ADDRESS_BITS - bct->logtwo((double)C) + bct->logtwo((double)A)-(int)(bct->logtwo(NSubbanks));
        tagbits = (int) (ADDRESS_BITS - bct->logtwo((double)C) + bct->logtwo((double)A) - (int)(bct->logtwo(NSubbanks)) + EPSILON);
        /*dt: we have to round up in case we have an odd number of bits in the tag, to account for the longer path */
        if ((tagbits % 4) == 0) {
            tagbits = tagbits / 4;
        }
        else {
            tagbits = tagbits / 4 + 1;
        }
    }
    else {
        tagbits = force_tag_size;
    }
    cols = 0;
    if (Ntwl*Ntbl == 1) {
        vert = 0;
        hori = 4 * tagbits * A * Ntspd / Ntwl;
    }
    else if (Ntwl*Ntbl == 2 || Ntwl*Ntbl == 4) {
        vert = 0;
        hori = 4 * tagbits * A * Ntspd / Ntwl;
    }
    else if (Ntwl*Ntbl > 4) {
        nr_subarrays_left = Ntwl * Ntbl;
        nr_subarrays_left /= 4;
        horizontal_step = 4 * tagbits * A * Ntspd / Ntwl;
        vertical_step = C / (B * A * Ntbl * Ntspd);
        hori = horizontal_step;
        horizontal_step *= 2;
        v_or_h = 1; // next step is vertical

        while (nr_subarrays_left > 1) {
            if (v_or_h) {
                vert += vertical_step;
                v_or_h = 0;
                vertical_step *= 2;
                nr_subarrays_left /= 2;
            }
            else {
                hori += horizontal_step;
                v_or_h = 1;
                horizontal_step *= 2;
                nr_subarrays_left /= 2;
            }
        }
    }

    /*dt: I think we need to add at least one buffer (probably more) between the comparator output gate and the mux driver ('cause it's biig!)*/


    Ceq = bct->gatecap(circuit_param_t::cparam_WmuxdrvNANDn + circuit_param_t::cparam_WmuxdrvNANDp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          hori * circuit_param_t::cparam_GlobalCwordmetal + vert * circuit_param_t::cparam_GlobalCbitmetal;
    Cload = Ceq / bct->gatecap(1.0, 0.0);
    Wxtramuxdrv2p = Cload * SizingRatio * 2 / 3;
    Wxtramuxdrv2n = Cload * SizingRatio / 3;

    /*dt: changing this to a nand*/
    Ceq = bct->gatecap(Wxtramuxdrv2n + Wxtramuxdrv2p, 20.0 / circuit_param_t::cparam_FUDGEFACTOR);
    Cload = Ceq / bct->gatecap(1.0, 0.0);
    Wxtramuxdrv1p = Cload * SizingRatio * 2 / 3;
    Wxtramuxdrv1n = 2 * Cload * SizingRatio / 3;

    r1 = bct->transreson(circuit_param_t::cparam_Wcompn, NCH, 2);
    r2 = bct->transreson(circuit_param_t::cparam_Wevalinvn, NCH, 1); /* was switch */
    c2 = (tagbits) * (bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 2)) +
         bct->draincap(circuit_param_t::cparam_Wevalinvp, PCH, 1) + bct->draincap(circuit_param_t::cparam_Wevalinvn, NCH, 1);
    c1 = (tagbits) * (bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wcompn, NCH, 2))
         + bct->draincap(circuit_param_t::cparam_Wcompp, PCH, 1) +
         + bct->gatecap(Wxtramuxdrv1n + Wxtramuxdrv1p, 5.0 / circuit_param_t::cparam_FUDGEFACTOR);

    dynPower += c2 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * 2 * A;
    dynPower += c1 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * (A - 1);

    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wevalinvn, circuit_param_t::cparam_Wevalinvp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    // double the tag_bits as the delay is being computed for only
    // half the number. The leakage is still for the total tag bits

    // the comparator compares cols number of bits. Since these transistors are
    // stacked, a stacking factor of 0.2 is used
    lkgCurrent += 2 * cols * 0.2 * 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wcompn, circuit_param_t::cparam_Wcompp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    // leakage due to the mux driver
    lkgCurrent += 0.5 * A * cmos_ileakage(circuit_param_t::cparam_Wmuxdrv12n, circuit_param_t::cparam_Wmuxdrv12p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    /* time to go to threshold of mux driver */

    tstep = (r2 * c2 + (r1 + r2) * c1) * log(1.0 / VTHMUXNAND);

    /* take into account non-zero input rise time */

    m = Vdd / nextinputtime;

    if ((tstep) <= (0.5*(Vdd - Vt) / m)) {
        a = m;
        b = 2 * ((Vdd * VTHEVALINV) - Vt);
        c = -2 * (tstep) * (Vdd - Vt) + 1 / m * ((Vdd * VTHEVALINV) - Vt) * ((Vdd * VTHEVALINV) - Vt);
        Tcomparatorni = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
    } else {
        Tcomparatorni = (tstep) + (Vdd + Vt) / (2 * m) - (Vdd * VTHEVALINV) / m;
    }
    nextinputtime = Tcomparatorni / (1.0 - VTHMUXNAND);
    //*outputtime = Tcomparatorni/(1.0-VTHMUXNAND);

    /*dt: first mux buffer */

    Ceq = bct->gatecap(Wxtramuxdrv2n + Wxtramuxdrv2p, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          2 * bct->draincap(Wxtramuxdrv1p, PCH, 1) + bct->draincap(Wxtramuxdrv1n, NCH, 1);
    Req = bct->transreson(Wxtramuxdrv1p, PCH, 1);
    tf = Req * Ceq;
    muxdrv1del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHCOMPINV, FALL);
    nextinputtime = muxdrv2del / VTHCOMPINV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent = 0.5 * A * cmos_ileakage(Wxtramuxdrv2n, Wxtramuxdrv2p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);


    /*dt: last mux buffer */

    Ceq = bct->gatecap(circuit_param_t::cparam_WmuxdrvNANDn + circuit_param_t::cparam_WmuxdrvNANDp, 20.0 / circuit_param_t::cparam_FUDGEFACTOR) +
          hori * circuit_param_t::cparam_GlobalCwordmetal + vert * circuit_param_t::cparam_GlobalCbitmetal +
          bct->draincap(Wxtramuxdrv2p, PCH, 1) + bct->draincap(Wxtramuxdrv2n, NCH, 1);
    Req = bct->transreson(Wxtramuxdrv2p, PCH, 1);
    tf = Req * Ceq;
    muxdrv2del = bct->horowitz(nextinputtime, tf, VTHCOMPINV, VTHCOMPINV, FALL);
    *outputtime = muxdrv1del / VTHCOMPINV;
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    lkgCurrent = 0.5 * A * cmos_ileakage(Wxtramuxdrv1n, Wxtramuxdrv1p, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);


    power->readOp.dynamic = dynPower;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    return(Tcomparatorni + st1del + st2del + st3del + muxdrv1del + muxdrv2del);
}
/*----------------------------------------------------------------------*/

/* Data output delay (data side) -- see section 6.8
   This is the time through the NAND/NOR gate and the final inverter
   assuming sel is already present */

double cacti_time_t::dataoutput_delay (int C, int B, int A, char fullyassoc, int Ndbl, double Nspd, int Ndwl,
                                       double inrisetime, double *outrisetime, powerDef *power)
{
    //double Ceq,Req,Rwire,Rline;
    double Ceq, Req, Rwire;
    double tf;
    double nordel, outdel, nextinputtime;
    double l_outdrv_v = 0, l_outdrv_h = 0;
    //int rows,cols,rows_fa_subarray,cols_fa_subarray;
    int rows_fa_subarray, cols_fa_subarray;
    //double htree, Cload;
    double htree;
    int htree_int, exp, tagbits;
    double lkgCurrent = 0.0, dynPower = 0.0;
    double this_delay;
    int i;
    /*dt: if we have fast cache mode, all A ways are routed to the edge of the data array*/
    int routed_ways = fast_cache_access_flag ? A : 1;

    if (!fullyassoc) {
    }
    else {
        rows_fa_subarray = (C / (B * Ndbl));
        if (!force_tag) {
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //tagbits = ADDRESS_BITS + EXTRA_TAG_BITS-bct->logtwo((double)B);
            tagbits = (int) (ADDRESS_BITS + EXTRA_TAG_BITS - bct->logtwo((double)B) + EPSILON);
        }
        else {
            tagbits = force_tag_size;
        }
        cols_fa_subarray = (CHUNKSIZE * B) + tagbits;
        if (Ndbl == 1) {
            l_outdrv_v = 0;
            l_outdrv_h = cols_fa_subarray;
        }
        if (Ndbl == 2 || Ndbl == 4) {
            l_outdrv_v = 0;
            l_outdrv_h = 2 * cols_fa_subarray;
        }
        if (Ndbl > 4) {
            htree = bct->logtwo((double)(Ndbl));
            //v4.1: Fixing double->int type conversion problems. EPSILON is added below to make sure
            //the final int value is the correct one
            //htree_int = (int) htree;
            htree_int = (int) (htree + EPSILON);
            if (htree_int % 2 == 0) {
                exp = (htree_int / 2 - 1);
                l_outdrv_v = (bct->powers(2, exp) - 1) * rows_fa_subarray;
                l_outdrv_h = sqrt((double)(Ndbl)) * cols_fa_subarray;
            }
            else {
                exp = (htree_int + 1) / 2 - 1;
                l_outdrv_v = (bct->powers(2, exp) - 1) * rows_fa_subarray;
                l_outdrv_h = sqrt((double)Ndbl / 2) * cols_fa_subarray;
            }
        }
    }
    /* Delay of NOR gate */

    Ceq = 2 * bct->draincap(circuit_param_t::cparam_Woutdrvnorn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Woutdrvnorp, PCH, 2) +
          bct->gatecap(circuit_param_t::cparam_Woutdrivern, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);

    tf = Ceq * bct->transreson(circuit_param_t::cparam_Woutdrvnorp, PCH, 2);
    nordel = bct->horowitz(inrisetime, tf, VTHOUTDRNOR, VTHOUTDRIVE, FALL);
    nextinputtime = nordel / (VTHOUTDRIVE);
    dynPower += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT;

    lkgCurrent += 0.5 * circuit_param_t::cparam_BITOUT * routed_ways * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Woutdrvseln, circuit_param_t::cparam_Woutdrvselp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    lkgCurrent += 0.5 * 0.2 * circuit_param_t::cparam_BITOUT * routed_ways * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Woutdrvnorn, circuit_param_t::cparam_Woutdrvnorp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    lkgCurrent += 0.5 * 0.2 * circuit_param_t::cparam_BITOUT * routed_ways * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Woutdrvnandn, circuit_param_t::cparam_Woutdrvnandp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);


    /* Delay of output driver tree*/

    if (nr_outdrvtreesegments) {
        Ceq = (bct->draincap(circuit_param_t::cparam_Woutdrivern, NCH, 1) + bct->draincap(circuit_param_t::cparam_Woutdriverp, PCH, 1)) * A * circuit_param_t::cparam_muxover
              + Coutdrvtreesegments[0] + bct->gatecap(3 * WoutdrvtreeN[1], 0.0);
    }
    else {
        Ceq = (bct->draincap(circuit_param_t::cparam_Woutdrivern, NCH, 1) + bct->draincap(circuit_param_t::cparam_Woutdriverp, PCH, 1)) * A * circuit_param_t::cparam_muxover
              + Coutdrvtreesegments[0] + bct->gatecap(circuit_param_t::cparam_Wsenseextdrv1n + circuit_param_t::cparam_Wsenseextdrv1p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    }
    Rwire = Routdrvtreesegments[0];

    dynPower += routed_ways * Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow * .5 * circuit_param_t::cparam_BITOUT;//factor of routed_ways added by Shyam

    lkgCurrent += 0.5 * circuit_param_t::cparam_BITOUT * routed_ways * circuit_param_t::cparam_muxover * cmos_ileakage(circuit_param_t::cparam_Woutdrivern, circuit_param_t::cparam_Woutdriverp, Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);

    tf = Ceq * (bct->transreson(circuit_param_t::cparam_Woutdrivern, NCH, 1) + Rwire);
    outdel = bct->horowitz(nextinputtime, tf, VTHOUTDRIVE, 0.5, RISE);

    *outrisetime = outdel / (1.0 - VTHOUTDRIVE);

    /*dt: doing all the H-tree segments*/

    if (nr_outdrvtreesegments) {
        for (i = 1; i < nr_outdrvtreesegments - 1;i++) {
            /*dt: this too should alternate...*/
            Ceq = (Coutdrvtreesegments[i] + bct->draincap(2 * WoutdrvtreeN[i], PCH, 1) + bct->draincap(WoutdrvtreeN[i], NCH, 1) +
                   bct->gatecap(3 * WoutdrvtreeN[i], 0.0));
            Req = (Routdrvtreesegments[i] + bct->transreson(WoutdrvtreeN[i], NCH, 1));
            tf = Req * Ceq;
            /*dt: This shouldn't be all falling, but interleaved. Have to fix that at some point.*/
            this_delay = bct->horowitz(*outrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);
            outdel += this_delay;
            *outrisetime = this_delay / (1.0 - VTHINV360x240);

            dynPower += routed_ways * circuit_param_t::cparam_BITOUT * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
            lkgCurrent += pow(2.00 , (double)nr_outdrvtreesegments - i) * routed_ways * circuit_param_t::cparam_BITOUT * 0.5 *
                          cmos_ileakage(WoutdrvtreeN[i], 2 * WoutdrvtreeN[i], Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
        }
        Ceq = Coutdrvtreesegments[nr_outdrvtreesegments-1] + bct->draincap(2 * WoutdrvtreeN[nr_outdrvtreesegments-1], PCH, 1) +
              bct->draincap(WoutdrvtreeN[nr_outdrvtreesegments-1], NCH, 1) + bct->gatecap(circuit_param_t::cparam_Wsenseextdrv1n + circuit_param_t::cparam_Wsenseextdrv1p, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
        Req = (Routdrvtreesegments[nr_outdrvtreesegments-1] + bct->transreson(WoutdrvtreeN[nr_outdrvtreesegments-1], NCH, 1));
        tf = Req * Ceq;
        /*dt: This shouldn't be all falling, but interleaved. Have to fix that at some point.*/
        this_delay = bct->horowitz(*outrisetime, tf, VTHINV360x240, VTHINV360x240, RISE);
        outdel += this_delay;
        *outrisetime = this_delay / (1.0 - VTHINV360x240);
        dynPower += routed_ways * circuit_param_t::cparam_BITOUT * Ceq * .5 * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
        lkgCurrent += routed_ways * circuit_param_t::cparam_BITOUT * 0.5 * cmos_ileakage(WoutdrvtreeN[nr_outdrvtreesegments-1], 2 * WoutdrvtreeN[nr_outdrvtreesegments-1],
                      Vt_bit_nmos_low, Vthn, Vt_bit_pmos_low, Vthp);
    }

    //*outrisetime = outdel/0.5;

    power->readOp.dynamic = dynPower;
    power->readOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    power->writeOp.dynamic = 0.0;
    power->writeOp.leakage = lkgCurrent * circuit_param_t::cparam_VddPow;

    return(outdel + nordel);
}

/*----------------------------------------------------------------------*/

/* Sel inverter delay (part of the output driver)  see section 6.8 */

double cacti_time_t::selb_delay_tag_path (double inrisetime, double *outrisetime, powerDef *power)
{
    double Ceq, Tst1, tf;

    Ceq = bct->draincap(circuit_param_t::cparam_Woutdrvseln, NCH, 1) + bct->draincap(circuit_param_t::cparam_Woutdrvselp, PCH, 1) +
          bct->gatecap (circuit_param_t::cparam_Woutdrvnandn + circuit_param_t::cparam_Woutdrvnandp, 10.0 / circuit_param_t::cparam_FUDGEFACTOR);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Woutdrvseln, NCH, 1);
    Tst1 = bct->horowitz (inrisetime, tf, VTHOUTDRINV, VTHOUTDRNAND, RISE);
    *outrisetime = Tst1 / (1.0 - VTHOUTDRNAND);
    power->readOp.dynamic += Ceq * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;

    return (Tst1);
}


/*----------------------------------------------------------------------*/

/* This routine calculates the extra time required after an access before
 * the next access can occur [ie.  it returns (cycle time-access time)].
 */

double cacti_time_t::precharge_delay (double worddata)
{
    double Ceq, tf, pretime;

    /* as discussed in the tech report, the delay is the delay of
       4 inverter delays (each with fanout of 4) plus the delay of
       the wordline */

    Ceq = bct->draincap(circuit_param_t::cparam_Wdecinvn, NCH, 1) + bct->draincap(circuit_param_t::cparam_Wdecinvp, PCH, 1) +
          4 * bct->gatecap (circuit_param_t::cparam_Wdecinvn + circuit_param_t::cparam_Wdecinvp, 0.0);
    tf = Ceq * bct->transreson (circuit_param_t::cparam_Wdecinvn, NCH, 1);
    pretime = 4 * bct->horowitz (0.0, tf, 0.5, 0.5, RISE) + worddata;

    return (pretime);
}

void cacti_time_t::calc_wire_parameters(parameter_type *parameters) {
    if (parameters->fully_assoc)
        /* If model is a fully associative cache - use larger cell size */
    {
        circuit_param_t::cparam_FACbitmetal =
            ((32 / circuit_param_t::cparam_FUDGEFACTOR +
              2 * circuit_param_t::cparam_WIREPITCH *
              ((parameters->
                num_write_ports + parameters->num_readwrite_ports - 1) +
               parameters->num_read_ports)) * Default_Cmetal);
        circuit_param_t::cparam_FACwordmetal =
            ((circuit_param_t::cparam_BitWidth +
              2 * circuit_param_t::cparam_WIREPITCH *
              ((parameters->
                num_write_ports + parameters->num_readwrite_ports - 1)) +
              circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports)) * Default_Cmetal);
        //v4.1: Fixing an error in the resistance equations below. WIREHEIGHTRATIO is replaced by
        //circuit_param_t::cparam_WIREWIDTH.
        /*circuit_param_t::cparam_FARbitmetal =
        (((32 +
        2 * circuit_param_t::cparam_WIREPITCH *
        ((parameters->
        num_write_ports + parameters->num_readwrite_ports - 1) +
        parameters->num_read_ports))) * 
        (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR));
        circuit_param_t::cparam_FARwordmetal =
        (((circuit_param_t::cparam_BitWidth +
        2 * circuit_param_t::cparam_WIREPITCH *
        ((parameters->
        num_write_ports + parameters->num_readwrite_ports - 1)) +
        circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports))) * (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR));*/

        circuit_param_t::cparam_FARbitmetal =
            (((32 / circuit_param_t::cparam_FUDGEFACTOR +
               2 * circuit_param_t::cparam_WIREPITCH *
               ((parameters->
                 num_write_ports + parameters->num_readwrite_ports - 1) +
                parameters->num_read_ports))) *
             (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREWIDTH * circuit_param_t::cparam_FUDGEFACTOR));
        circuit_param_t::cparam_FARwordmetal =
            (((circuit_param_t::cparam_BitWidth +
               2 * circuit_param_t::cparam_WIREPITCH *
               ((parameters->
                 num_write_ports + parameters->num_readwrite_ports - 1)) +
               circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports))) *
             (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREWIDTH * circuit_param_t::cparam_FUDGEFACTOR));
    }
    /*dt: Capacitance gets better with scaling, because C = K*epsilon*L*(W/x_ox + H/L_s), where
          W: width of the wire, L: length of the wire, H: height of the wire, x_ox: thickness of the oxide separating two wires
    	  vertically, L_s: thickness of the insulator separating two wires horizontally 
    	  K and epsilon are constants
    	  To a first order all the variables scale with featuresize => C~ const. * f^2/f = const. * f 
    	  Note that circuit_param_t::cparam_FUDGEFACTOR is the inverse of featuresize
    	  Since we're going to divide by circuit_param_t::cparam_FUDGEFACTOR in the end anyway, I'm leaving this alone and just adding this comment for future reference.
       */
    circuit_param_t::cparam_Cbitmetal =
        ((circuit_param_t::cparam_BitHeight +
          2 * circuit_param_t::cparam_WIREPITCH *
          ((parameters->num_write_ports + parameters->num_readwrite_ports - 1) +
           parameters->num_read_ports)) * Default_Cmetal);
    circuit_param_t::cparam_Cwordmetal =
        ((circuit_param_t::cparam_BitWidth +
          2 * circuit_param_t::cparam_WIREPITCH *
          ((parameters->num_write_ports + parameters->num_readwrite_ports - 1)) +
          circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports)) * Default_Cmetal);
    /* dt: Resistance gets worse, since R = rho*L/(W*H), where L, W, H are the length, width and height of the wire.
       All scale with featuresize, so R ~ const*f/f^2 = const./f
    Note that circuit_param_t::cparam_FUDGEFACTOR is the inverse of featuresize
    */

    //v4.1: Fixing an error in the resistance equations below. WIREHEIGHTRATIO is replaced by
    // circuit_param_t::cparam_WIREWIDTH.
    /*circuit_param_t::cparam_Rbitmetal =
      (((circuit_param_t::cparam_BitHeight +
         2 * circuit_param_t::cparam_WIREPITCH *
         ((parameters->num_write_ports + parameters->num_readwrite_ports - 1) + parameters->num_read_ports))) * 
      (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR));
    circuit_param_t::cparam_Rwordmetal =
      (((circuit_param_t::cparam_BitWidth +
         2 * circuit_param_t::cparam_WIREPITCH *
         ((parameters->num_write_ports + parameters->num_readwrite_ports - 1)) + circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports))) * 
      (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR));*/

    circuit_param_t::cparam_Rbitmetal =
        (((circuit_param_t::cparam_BitHeight +
           2 * circuit_param_t::cparam_WIREPITCH *
           ((parameters->num_write_ports + parameters->num_readwrite_ports - 1) + parameters->num_read_ports))) *
         (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREWIDTH * circuit_param_t::cparam_FUDGEFACTOR));
    circuit_param_t::cparam_Rwordmetal =
        (((circuit_param_t::cparam_BitWidth +
           2 * circuit_param_t::cparam_WIREPITCH *
           ((parameters->num_write_ports + parameters->num_readwrite_ports - 1)) + circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports))) *
         (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREWIDTH * circuit_param_t::cparam_FUDGEFACTOR));

    /*dt: tags only need one RW port for when a line is evicted. All other ports can be simple read ports */
    circuit_param_t::cparam_TagCbitmetal = (circuit_param_t::cparam_BitHeight + 2 * circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + parameters->num_readwrite_ports - 1)) *
                                           Default_Cmetal;
    /*dt: the +1 is because a RW port needs two bitlines, thus longer wordlines */
    circuit_param_t::cparam_TagCwordmetal = (circuit_param_t::cparam_BitWidth + circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + 2 * (parameters->num_readwrite_ports - 1))) *
                                            Default_Cmetal;
    /*circuit_param_t::cparam_TagRbitmetal = (circuit_param_t::cparam_BitHeight + 2 * circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + parameters->num_readwrite_ports - 1))  *
    		 (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_TagRwordmetal = (circuit_param_t::cparam_BitWidth + circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + 2*(parameters->num_readwrite_ports - 1))) * 
                 (Default_CopperSheetResistancePerMicroM /circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR);*/

    circuit_param_t::cparam_TagRbitmetal = (circuit_param_t::cparam_BitHeight + 2 * circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + parameters->num_readwrite_ports - 1))  *
                                           (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR * circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_TagRwordmetal = (circuit_param_t::cparam_BitWidth + circuit_param_t::cparam_WIREPITCH * (parameters->num_read_ports + 2 * (parameters->num_readwrite_ports - 1))) *
                                            (Default_CopperSheetResistancePerMicroM / circuit_param_t::cparam_WIREHEIGTHRATIO * circuit_param_t::cparam_FUDGEFACTOR * circuit_param_t::cparam_FUDGEFACTOR);

    circuit_param_t::cparam_GlobalCbitmetal = CRatiolocal_to_interm * circuit_param_t::cparam_Cbitmetal;
    circuit_param_t::cparam_GlobalCwordmetal = CRatiolocal_to_interm * circuit_param_t::cparam_Cwordmetal;

    circuit_param_t::cparam_GlobalRbitmetal = RRatiolocal_to_interm * circuit_param_t::cparam_Rbitmetal;
    circuit_param_t::cparam_GlobalRwordmetal = RRatiolocal_to_interm * circuit_param_t::cparam_Rwordmetal;
}

/*======================================================================*/


void cacti_time_t::calculate_time (result_type *result, arearesult_type *arearesult,
                                   area_type *arearesult_subbanked, parameter_type *parameters,
                                   double *NSubbanks)
{
    arearesult_type arearesult_temp;
    area_type arearesult_subbanked_temp;

    int i;
    //int Ndwl, Ndbl, Ntwl, Ntbl, Ntspd, inter_subbanks, rows, columns,
    //tag_driver_size1, tag_driver_size2;
    int Ndwl, Ndbl, Ntwl, Ntbl, Ntspd, rows, columns;
    double Nspd;
    double Subbank_Efficiency, Total_Efficiency, max_efficiency, efficiency,
    min_efficiency;
    double max_aspect_ratio_total, aspect_ratio_total_temp;
    //double bank_h, bank_v, subbank_h, subbank_v, inter_h, inter_v;
    double bank_h, bank_v, subbank_h, subbank_v;
    double wirelength_v, wirelength_h;
    double access_time = 0;
    powerDef total_power;
    double before_mux = 0.0, after_mux = 0;
    powerDef total_power_allbanks;
    powerDef total_address_routing_power, total_power_without_routing;
    double subbank_address_routing_delay = 0.0;
    powerDef subbank_address_routing_power;
    double decoder_data_driver = 0.0, decoder_data_3to8 =
                                     0.0, decoder_data_inv = 0.0;
    double decoder_data = 0.0, decoder_tag = 0.0, wordline_data =
                              0.0, wordline_tag = 0.0;
    powerDef decoder_data_power, decoder_tag_power, wordline_data_power, wordline_tag_power;
    double decoder_tag_driver = 0.0, decoder_tag_3to8 = 0.0, decoder_tag_inv =
                                    0.0;
    double bitline_data = 0.0, bitline_tag = 0.0, sense_amp_data =
                              0.0, sense_amp_tag = 0.0;
    powerDef sense_amp_data_power, sense_amp_tag_power, bitline_tag_power, bitline_data_power;
    double compare_tag = 0.0, mux_driver = 0.0, data_output = 0.0, selb = 0.0;
    powerDef compare_tag_power, selb_power, mux_driver_power, valid_driver_power;
    powerDef data_output_power;
    double time_till_compare = 0.0, time_till_select = 0.0, valid_driver = 0.0;
    double cycle_time = 0.0, precharge_del = 0.0;
    double outrisetime = 0.0, inrisetime = 0.0, addr_inrisetime = 0.0;
    double senseext_scale = 1.0;
    double total_out_driver = 0.0;
    powerDef total_out_driver_power;
    double scale_init;
    int data_nor_inputs = 1, tag_nor_inputs = 1;
    double tag_delay_part1 = 0.0, tag_delay_part2 =
                                 0.0, tag_delay_part3 = 0.0, tag_delay_part4 = 0.0, tag_delay_part5 =
                                                            0.0, tag_delay_part6 = 0.0;
    double max_delay = 0;
    int counter;
    double Tpre, Tpre_tag;
    int allports = 0, allreadports = 0, allwriteports = 0;
    int tag_iteration_counter = 0;
    double colsfa, desiredrisetimefa, Clinefa, Rpdrivefa; //Added by Shyam to incorporate FA caches
    //in v4.0


    cached_tag_entry cached_tag_calculations[MAX_CACHE_ENTRIES];
    cached_tag_entry current_cache_entry;

    int valid_tag_cache_entries[MAX_CACHE_ENTRIES];

    for (i = 0;i < MAX_CACHE_ENTRIES;i++) {
        valid_tag_cache_entries[i] = FALSE;
    }

    rows = parameters->number_of_sets;
    columns = 8 * parameters->block_size * parameters->data_associativity;
    
    /*dt: added to simulate accessing the tags and then only accessing one data way*/
    sequential_access_flag = parameters->sequential_access;
    /*dt: added to simulate fast and power hungry caches which route all the ways to the edge of the
          data array and do the way select there, instead of waiting for the way select signal to travel
    all the way to the subarray.
    */
    fast_cache_access_flag = parameters->fast_access;
    area_param_t::aparam_pure_sram_flag = parameters->pure_sram;
    /* how many bits should this cache output?*/
    circuit_param_t::cparam_BITOUT = parameters->nr_bits_out;

    if (circuit_param_t::cparam_VddPow < 0.7)
        circuit_param_t::cparam_VddPow = 0.7;
    if (circuit_param_t::cparam_VddPow > 5.0)
        circuit_param_t::cparam_VddPow = 5.0;
    VbitprePow = circuit_param_t::cparam_VddPow * 3.3 / 4.5;
    parameters->VddPow = circuit_param_t::cparam_VddPow;

    /* go through possible Ndbl,Ndwl and find the smallest */
    /* Because of area considerations, I don't think it makes sense
       to break either dimension up larger than MAXN */

    /* only try moving output drivers for set associative cache */
    if (parameters->tag_associativity != 1) {
        scale_init = 0.1;
    }
    else {
        scale_init = 1.0;
    }
    leakage_obj->precalc_leakage_params(circuit_param_t::cparam_VddPow, Tox);
    calc_wire_parameters(parameters);

    result->cycle_time = BIGNUM;
    result->access_time = BIGNUM;
    result->total_power.readOp.dynamic = result->total_power.readOp.leakage =
                                             result->total_power.writeOp.dynamic = result->total_power.writeOp.leakage = BIGNUM;
    result->best_muxover = 8 * parameters->block_size / circuit_param_t::cparam_BITOUT;
    result->max_access_time = 0;
    result->max_power = 0;
    arearesult_temp.max_efficiency = 1.0 / BIGNUM;
    max_efficiency = 1.0 / BIGNUM;
    min_efficiency = BIGNUM;
    arearesult->efficiency = 1.0 / BIGNUM;
    max_aspect_ratio_total = 1.0 / BIGNUM;
    arearesult->aspect_ratio_total = BIGNUM;



    for (counter = 0; counter < 2; counter++)
    {
        if (!parameters->fully_assoc)
        {
            /* Set associative or direct map cache model */

            /*
            Varies the number of sets mapped to each wordline. 
            This makes each wordline longer but reduces the length of the bitlines.
            It also requires extra muxes before the sense amplifiers to select the right set.
            */
            //v4.1: We now consider fractions of Nspd while exploring the partitioning
            //space. The lower bound of Nspd is circuit_param_t::cparam_BITOUT /(parameters->block_size*8).
            for (Nspd = (double)(circuit_param_t::cparam_BITOUT) / (double)(parameters->block_size * 8); Nspd <= MAXDATASPD; Nspd = Nspd * 2)
            {
                /*
                Varies the number of wordline segments, 
                i.e. into how many "sub"-wordlines each wordline is split. 
                Increasing the number of segments, increases the number of wordline drivers, 
                but makes each segment shorter and faster.
                */
                for (Ndwl = 1; Ndwl <= MAXDATAN; Ndwl = Ndwl * 2)
                {
                    /*
                    Varies the number of bitline segments,
                    i.e. into how many "sub"-bitlines each bitline is split.
                    Increasing the number of segments increases the number of sense amplifiers needed, 
                    but makes each bitline segment shorter and faster. 
                    */
                    for (Ndbl = 1; Ndbl <= MAXDATAN; Ndbl = Ndbl * 2)
                    {
                        /*
                        Varies the number of sets per wordline, but for the tag array
                        */
                        if (area_obj->data_organizational_parameters_valid(parameters->block_size,
                                parameters->tag_associativity,
                                parameters->cache_size,
                                Ndwl, Ndbl, Nspd,
                                parameters->fully_assoc, (*NSubbanks)))
                        {
                            tag_iteration_counter = 0;
                            bank_h = 0;
                            bank_v = 0;

                            if (8 * parameters->block_size /
                                    circuit_param_t::cparam_BITOUT == 1 && Nspd == 1)
                            {
                                circuit_param_t::cparam_muxover = 1;
                            }
                            else {
                                if (Nspd > MAX_COL_MUX)
                                {
                                    circuit_param_t::cparam_muxover = 8 * parameters->block_size / circuit_param_t::cparam_BITOUT;
                                }
                                else {
                                    if (8 * parameters->block_size *
                                            Nspd / circuit_param_t::cparam_BITOUT >
                                            MAX_COL_MUX)
                                    {
                                        circuit_param_t::cparam_muxover = (8 * parameters->block_size / circuit_param_t::cparam_BITOUT) /
                                                                          (MAX_COL_MUX / (Nspd));
                                    }
                                    else {
                                        circuit_param_t::cparam_muxover = 1;
                                    }
                                }
                            }

                            reset_data_device_widths();
                            compute_device_widths(parameters->cache_size, parameters->block_size, parameters->tag_associativity,
                                                  parameters->fully_assoc, Ndwl, Ndbl, Nspd);
                            if (*NSubbanks == 1.0) {

                                reset_powerDef(&total_address_routing_power);
                                reset_powerDef(&subbank_address_routing_power);

                                inrisetime = addr_inrisetime = 0;

                                /* Calculate data side of cache */
                                Tpre = 0;

                                max_delay = 0;
                                reset_powerDef(&decoder_data_power);
                                decoder_data = decoder_delay (parameters->cache_size,
                                                   parameters->block_size,
                                                   parameters->data_associativity,
                                                   Ndwl, Ndbl, Nspd,
                                                   *NSubbanks,
                                                   &decoder_data_driver,
                                                   &decoder_data_3to8,
                                                   &decoder_data_inv,
                                                   inrisetime,
                                                   &outrisetime,
                                                   &data_nor_inputs,
                                                   &decoder_data_power);

                                Tpre = decoder_data;
                                max_delay = MAX (max_delay, decoder_data);
                                inrisetime = outrisetime;

                                reset_powerDef(&wordline_data_power);
                                wordline_data =
                                    wordline_delay (
                                        parameters->cache_size,
                                        parameters->block_size,
                                        parameters->data_associativity,
                                        Ndwl, Ndbl, Nspd,
                                        inrisetime,
                                        &outrisetime,
                                        &wordline_data_power);

                                inrisetime = outrisetime;
                                max_delay = MAX (max_delay, wordline_data);
                                /*dt: assuming precharge delay is decoder + wordline delay */
                                Tpre += wordline_data;

                                reset_powerDef(&bitline_data_power);
                                bitline_data =
                                    bitline_delay (parameters->cache_size,
                                                   parameters->data_associativity,
                                                   parameters->block_size,
                                                   Ndwl, Ndbl, Nspd,
                                                   inrisetime,
                                                   &outrisetime,
                                                   &bitline_data_power, Tpre);



                                inrisetime = outrisetime;
                                max_delay = MAX (max_delay, bitline_data);


                                reset_powerDef(&sense_amp_data_power);
                                sense_amp_data =
                                    sense_amp_delay (parameters->cache_size,
                                                     parameters->block_size,
                                                     parameters->data_associativity,
                                                     Ndwl, Ndbl, Nspd,
                                                     inrisetime,
                                                     &outrisetime,
                                                     &sense_amp_data_power);

                                max_delay = MAX (max_delay, sense_amp_data);


                                inrisetime = outrisetime;

                                reset_powerDef(&data_output_power);
                                data_output =
                                    dataoutput_delay (
                                        parameters->cache_size,
                                        parameters->block_size,
                                        parameters->data_associativity,
                                        parameters->fully_assoc,
                                        Ndbl, Nspd, Ndwl,
                                        inrisetime,
                                        &outrisetime,
                                        &data_output_power);

                                max_delay = MAX (max_delay, data_output);
                                inrisetime = outrisetime;

                                reset_powerDef(&total_out_driver_power);

                                subbank_v = 0;
                                subbank_h = 0;

                                inrisetime = outrisetime;
                                max_delay = MAX (max_delay, total_out_driver);

                                /* if the associativity is 1, the data output can come right
                                       after the sense amp.   Otherwise, it has to wait until 
                                       the data access has been done. 
                                */
                                if (parameters->data_associativity == 1)
                                {
                                    before_mux =
                                        subbank_address_routing_delay +
                                        decoder_data + wordline_data +
                                        bitline_data + sense_amp_data +
                                        total_out_driver + data_output;
                                    after_mux = 0;
                                }
                                else {
                                    before_mux =
                                        subbank_address_routing_delay +
                                        decoder_data + wordline_data +
                                        bitline_data + sense_amp_data;
                                    after_mux =
                                        data_output + total_out_driver;
                                }
                            }

                            for (Ntspd = 1; Ntspd <= MAXTAGSPD; Ntspd = Ntspd * 2)
                            {
                                /*
                                Varies the number of wordline segments, but for the tag array
                                */
                                /*dt: Ntwl is no longer limited to one. We assume a wire-or for the valid and select lines. */
                                for (Ntwl = 1; Ntwl <= MAXTAGN; Ntwl = Ntwl * 2)
                                {
                                    /*
                                    Varies the number of bitline segments, but for the array
                                    */
                                    for (Ntbl = 1; Ntbl <= MAXTAGN; Ntbl = Ntbl * 2)
                                    {

                                        if (area_obj->tag_organizational_parameters_valid(parameters->block_size,
                                                parameters->tag_associativity,
                                                parameters->cache_size,
                                                Ntwl, Ntbl, Ntspd,
                                                parameters->fully_assoc, (*NSubbanks)))
                                        {
                                            tag_iteration_counter++;

                                            reset_tag_device_widths();

                                            compute_tag_device_widths(parameters->cache_size, parameters->block_size, parameters->tag_associativity,
                                                                      Ntspd, Ntwl, Ntbl, (*NSubbanks));

                                            area_obj->area_subbanked(ADDRESS_BITS, circuit_param_t::cparam_BITOUT,
                                                                     parameters->num_readwrite_ports,
                                                                     parameters->num_read_ports,
                                                                     parameters->num_write_ports,
                                                                     Ndbl, Ndwl, Nspd,
                                                                     Ntbl, Ntwl, Ntspd,
                                                                     *NSubbanks,
                                                                     parameters,
                                                                     &arearesult_subbanked_temp,
                                                                     &arearesult_temp);

                                            Subbank_Efficiency =
                                                (area_param_t::aparam_area_all_dataramcells +
                                                 area_param_t::aparam_area_all_tagramcells) * 100 /
                                                (arearesult_temp.totalarea /100000000.0);

                                            //v4.1: No longer using calculate_area function as area has already been
                                            //computed for the given tech node
                                            /*Total_Efficiency =
                                            (*NSubbanks) *
                                            (area_param_t::aparam_area_all_dataramcells +
                                            area_param_t::aparam_area_all_tagramcells) * 100 /
                                            (calculate_area
                                            (arearesult_subbanked_temp,
                                            parameters->fudgefactor) /
                                            100000000.0);*/

                                            Total_Efficiency =
                                                (*NSubbanks) *
                                                (area_param_t::aparam_area_all_dataramcells +
                                                 area_param_t::aparam_area_all_tagramcells) * 100 /
                                                (arearesult_subbanked_temp.height * arearesult_subbanked_temp.width /
                                                 100000000.0);

                                            //efficiency = Subbank_Efficiency;
                                            efficiency = Total_Efficiency;

                                            arearesult_temp.efficiency = efficiency;
                                            aspect_ratio_total_temp =
                                                (arearesult_subbanked_temp.height /
                                                 arearesult_subbanked_temp.width);
                                            aspect_ratio_total_temp =
                                                (aspect_ratio_total_temp > 1.0) ?
                                                (aspect_ratio_total_temp) : 1.0 / (aspect_ratio_total_temp);

                                            arearesult_temp.aspect_ratio_total = aspect_ratio_total_temp;

                                            if (*NSubbanks > 1) {

                                                subbank_dim (parameters->cache_size,
                                                             parameters->block_size,
                                                             parameters->data_associativity,
                                                             parameters->fully_assoc,
                                                             Ndbl, Ndwl, Nspd, Ntbl,
                                                             Ntwl, Ntspd, *NSubbanks,
                                                             &bank_h, &bank_v);

                                                reset_powerDef(&total_address_routing_power);

                                                subbanks_routing_power (
                                                    parameters->fully_assoc,
                                                    parameters->data_associativity,
                                                    *NSubbanks,
                                                    &bank_h,
                                                    &bank_v,
                                                    &total_address_routing_power);

                                                reset_powerDef(&subbank_address_routing_power);
                                                /*dt: this has to be reset on every loop iteration, or else we implicitly reuse the risetime from the end
                                                of the last loop iteration!*/
                                                inrisetime = addr_inrisetime = 0;
                                                if (*NSubbanks > 2)
                                                {
                                                    subbank_address_routing_delay =
                                                        address_routing_delay
                                                        (parameters->cache_size,
                                                         parameters->block_size,
                                                         parameters->data_associativity,
                                                         parameters->fully_assoc, Ndwl,
                                                         Ndbl, Nspd, Ntwl, Ntbl, Ntspd,
                                                         NSubbanks, &outrisetime,
                                                         &subbank_address_routing_power);

                                                    /*dt: moved this here, because we only have extra signal slope if there's
                                                    something before the decoder */
                                                    inrisetime = outrisetime;
                                                    addr_inrisetime = outrisetime;
                                                }


                                                /* Calculate data side of cache */
                                                Tpre = 0;

                                                max_delay = 0;
                                                reset_powerDef(&decoder_data_power);
                                                decoder_data =
                                                    decoder_delay (parameters->cache_size,
                                                                   parameters->block_size,
                                                                   parameters->data_associativity,
                                                                   Ndwl, Ndbl, Nspd,
                                                                   *NSubbanks,
                                                                   &decoder_data_driver,
                                                                   &decoder_data_3to8,
                                                                   &decoder_data_inv,
                                                                   inrisetime,
                                                                   &outrisetime,
                                                                   &data_nor_inputs,
                                                                   &decoder_data_power);

                                                Tpre = decoder_data;
                                                max_delay = MAX (max_delay, decoder_data);
                                                inrisetime = outrisetime;

                                                reset_powerDef(&wordline_data_power);
                                                wordline_data =
                                                    wordline_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->data_associativity,
                                                        Ndwl, Ndbl, Nspd,
                                                        inrisetime,
                                                        &outrisetime,
                                                        &wordline_data_power);

                                                inrisetime = outrisetime;
                                                max_delay = MAX (max_delay, wordline_data);
                                                /*dt: assuming precharge delay is decoder + wordline delay */
                                                Tpre += wordline_data;

                                                reset_powerDef(&bitline_data_power);
                                                bitline_data =
                                                    bitline_delay (parameters->cache_size,
                                                                   parameters->data_associativity,
                                                                   parameters->block_size,
                                                                   Ndwl, Ndbl, Nspd,
                                                                   inrisetime,
                                                                   &outrisetime,
                                                                   &bitline_data_power, Tpre);



                                                inrisetime = outrisetime;
                                                max_delay = MAX (max_delay, bitline_data);


                                                reset_powerDef(&sense_amp_data_power);
                                                sense_amp_data =
                                                    sense_amp_delay (parameters->cache_size,
                                                                     parameters->block_size,
                                                                     parameters->data_associativity,
                                                                     Ndwl, Ndbl, Nspd,
                                                                     inrisetime,
                                                                     &outrisetime,
                                                                     &sense_amp_data_power);

                                                max_delay = MAX (max_delay, sense_amp_data);


                                                inrisetime = outrisetime;

                                                reset_powerDef(&data_output_power);
                                                data_output =
                                                    dataoutput_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->data_associativity,
                                                        parameters->fully_assoc,
                                                        Ndbl, Nspd, Ndwl,
                                                        inrisetime,
                                                        &outrisetime,
                                                        &data_output_power);

                                                max_delay = MAX (max_delay, data_output);
                                                inrisetime = outrisetime;

                                                reset_powerDef(&total_out_driver_power);

                                                subbank_v = 0;
                                                subbank_h = 0;


                                                subbank_routing_length (
                                                    parameters->cache_size,
                                                    parameters->block_size,
                                                    parameters->data_associativity,
                                                    parameters->fully_assoc,
                                                    Ndbl, Nspd,
                                                    Ndwl, Ntbl,
                                                    Ntwl, Ntspd,
                                                    *NSubbanks,
                                                    &subbank_v,
                                                    &subbank_h);

                                                if (*NSubbanks > 2)
                                                {
                                                    total_out_driver =
                                                        senseext_driver_delay
                                                        (
                                                            parameters->data_associativity,
                                                            parameters->fully_assoc,
                                                            inrisetime, &outrisetime,
                                                            subbank_v, subbank_h,
                                                            &total_out_driver_power);
                                                }

                                                inrisetime = outrisetime;
                                                max_delay = MAX (max_delay, total_out_driver);

                                                /* if the associativity is 1, the data output can come right
                                                   after the sense amp.   Otherwise, it has to wait until 
                                                   the data access has been done. */

                                                if (parameters->data_associativity == 1)
                                                {
                                                    before_mux =
                                                        subbank_address_routing_delay +
                                                        decoder_data + wordline_data +
                                                        bitline_data + sense_amp_data +
                                                        total_out_driver + data_output;
                                                    after_mux = 0;
                                                }
                                                else
                                                {
                                                    before_mux =
                                                        subbank_address_routing_delay +
                                                        decoder_data + wordline_data +
                                                        bitline_data + sense_amp_data;
                                                    after_mux =
                                                        data_output + total_out_driver;
                                                }
                                            }

                                            /*
                                             * Now worry about the tag side.
                                             */


                                            reset_powerDef(&decoder_tag_power);
                                            if (tag_iteration_counter < MAX_CACHE_ENTRIES && valid_tag_cache_entries[tag_iteration_counter] == TRUE) {
                                                current_cache_entry = cached_tag_calculations[tag_iteration_counter];
                                                decoder_tag = current_cache_entry.decoder.delay;
                                                copy_powerDef(&decoder_tag_power, current_cache_entry.decoder.power);
                                            }
                                            else {
                                                decoder_tag =
                                                    decoder_tag_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->tag_associativity,
                                                        Ntwl, Ntbl, Ntspd,
                                                        *NSubbanks,
                                                        &decoder_tag_driver,
                                                        &decoder_tag_3to8,
                                                        &decoder_tag_inv,
                                                        addr_inrisetime,
                                                        &outrisetime,
                                                        &tag_nor_inputs,
                                                        &decoder_tag_power);

                                                cached_tag_calculations[tag_iteration_counter].decoder.delay = decoder_tag;
                                                copy_powerDef(
                                                    &(cached_tag_calculations[tag_iteration_counter].decoder.power)
                                                    , decoder_tag_power);



                                            }
                                            max_delay =
                                                MAX (max_delay, decoder_tag);
                                            Tpre_tag = decoder_tag;
                                            inrisetime = outrisetime;

                                            reset_powerDef(&wordline_tag_power);
                                            if (tag_iteration_counter < MAX_CACHE_ENTRIES && valid_tag_cache_entries[tag_iteration_counter] == TRUE) {
                                                current_cache_entry = cached_tag_calculations[tag_iteration_counter];
                                                wordline_tag = current_cache_entry.wordline.delay;
                                                copy_powerDef(&wordline_tag_power, current_cache_entry.wordline.power);
                                            }
                                            else {
                                                wordline_tag =
                                                    wordline_tag_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->tag_associativity,
                                                        Ntspd, Ntwl, Ntbl,
                                                        *NSubbanks,
                                                        inrisetime,
                                                        &outrisetime,
                                                        &wordline_tag_power);

                                                cached_tag_calculations[tag_iteration_counter].wordline.delay = wordline_tag;
                                                copy_powerDef(
                                                    &(cached_tag_calculations[tag_iteration_counter].wordline.power)
                                                    , wordline_tag_power);
                                            }
                                            max_delay =
                                                MAX (max_delay, wordline_tag);
                                            inrisetime = outrisetime;
                                            Tpre_tag += wordline_tag;

                                            reset_powerDef(&bitline_tag_power);
                                            if (tag_iteration_counter < MAX_CACHE_ENTRIES && valid_tag_cache_entries[tag_iteration_counter] == TRUE ) {
                                                current_cache_entry = cached_tag_calculations[tag_iteration_counter];
                                                bitline_tag = current_cache_entry.bitline.delay;
                                                copy_powerDef(&bitline_tag_power, current_cache_entry.bitline.power);
                                            }
                                            else {
                                                bitline_tag =
                                                    bitline_tag_delay (
                                                        parameters->cache_size,
                                                        parameters->tag_associativity,
                                                        parameters->block_size,
                                                        Ntwl, Ntbl, Ntspd,
                                                        *NSubbanks,
                                                        inrisetime,
                                                        &outrisetime,
                                                        &bitline_tag_power, Tpre_tag);

                                                cached_tag_calculations[tag_iteration_counter].bitline.delay = bitline_tag;
                                                copy_powerDef(
                                                    &(cached_tag_calculations[tag_iteration_counter].bitline.power)
                                                    , bitline_tag_power);
                                            }
                                            max_delay =
                                                MAX (max_delay, bitline_tag);
                                            inrisetime = outrisetime;

                                            reset_powerDef(&sense_amp_tag_power);
                                            if (valid_tag_cache_entries[tag_iteration_counter] == TRUE) {
                                                current_cache_entry = cached_tag_calculations[tag_iteration_counter];
                                                sense_amp_tag = current_cache_entry.senseamp.delay;
                                                copy_powerDef(&sense_amp_tag_power, current_cache_entry.senseamp.power);

                                                outrisetime = current_cache_entry.senseamp_outputrisetime;
                                            }
                                            else {
                                                sense_amp_tag =
                                                    sense_amp_tag_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->tag_associativity,
                                                        Ntwl, Ntbl, Ntspd, *NSubbanks,
                                                        inrisetime, &outrisetime,
                                                        &sense_amp_tag_power);

                                                cached_tag_calculations[tag_iteration_counter].senseamp.delay = sense_amp_tag;
                                                copy_powerDef(
                                                    &(cached_tag_calculations[tag_iteration_counter].senseamp.power)
                                                    , sense_amp_tag_power);

                                                cached_tag_calculations[tag_iteration_counter].senseamp_outputrisetime = outrisetime;

                                                if (*NSubbanks == 1 && tag_iteration_counter < MAX_CACHE_ENTRIES) {
                                                    valid_tag_cache_entries[tag_iteration_counter] = TRUE;
                                                }
                                            }

                                            max_delay = MAX (max_delay, sense_amp_tag);
                                            inrisetime = outrisetime;

                                            wirelength_v = wirelength_h = 0;
                                            if (parameters->tag_associativity != 1) {
                                                precalc_muxdrv_widths(parameters->cache_size,
                                                                      parameters->block_size,
                                                                      parameters->data_associativity,
                                                                      Ndwl, Ndbl, Nspd,
                                                                      &wirelength_v, &wirelength_h);
                                            }

                                            /* split comparator - look at half the address bits only */
                                            reset_powerDef(&compare_tag_power);
                                            compare_tag =
                                                half_compare_delay (
                                                    parameters->cache_size,
                                                    parameters->block_size,
                                                    parameters->tag_associativity,
                                                    Ntwl, Ntbl, Ntspd,
                                                    *NSubbanks,
                                                    inrisetime,
                                                    &outrisetime,
                                                    &compare_tag_power);

                                            inrisetime = outrisetime;
                                            max_delay = MAX (max_delay, compare_tag);

                                            reset_powerDef(&valid_driver_power);
                                            reset_powerDef(&mux_driver_power);
                                            reset_powerDef(&selb_power);
                                            if (parameters->tag_associativity == 1)
                                            {
                                                mux_driver = 0;
                                                valid_driver =
                                                    valid_driver_delay (
                                                        parameters->cache_size,
                                                        parameters->block_size,
                                                        parameters->tag_associativity,
                                                        parameters->fully_assoc,
                                                        Ndbl, Ndwl,
                                                        Nspd, Ntbl,
                                                        Ntwl, Ntspd,
                                                        NSubbanks,
                                                        inrisetime,
                                                        &valid_driver_power);

                                                max_delay =
                                                    MAX (max_delay, valid_driver);
                                                time_till_compare =
                                                    subbank_address_routing_delay +
                                                    decoder_tag + wordline_tag +
                                                    bitline_tag + sense_amp_tag;

                                                time_till_select =
                                                    time_till_compare + compare_tag +
                                                    valid_driver;


                                                /*
                                                 * From the above info, calculate the total access time
                                                 */
                                                /*If we're doing a pure SRAM table, then we don't need to take into account tags */
                                                if (area_param_t::aparam_pure_sram_flag) {
                                                    access_time = before_mux + after_mux;
                                                }
                                                else {
                                                    if (sequential_access_flag) {
                                                        /*dt: testing sequential access mode*/
                                                        /*approximating the time it takes the match and valid signal to travel to the edge of the tag array with decode_tag */
                                                        access_time = before_mux + time_till_compare + compare_tag + decoder_tag;
                                                    }
                                                    else {
                                                        access_time =
                                                            MAX (before_mux + after_mux,
                                                                 time_till_select);
                                                    }
                                                }

                                            }
                                            else
                                            {


                                                if (fast_cache_access_flag || sequential_access_flag) {
                                                    mux_driver = 0;
                                                }//||sequential_access_flag added by Shyam to fix the bug
                                                //of mux driver delay being larger than access time in serial access mode
                                                //now mux driver delay code won't be touched in serial access mode
                                                else {
                                                    /* dualin mux driver - added for split comparator
                                                       - inverter replaced by nand gate */
                                                    mux_driver =
                                                        mux_driver_delay_dualin
                                                        (parameters->cache_size,
                                                         parameters->block_size,
                                                         parameters->tag_associativity,
                                                         Ntbl, Ntspd,
                                                         inrisetime, &outrisetime,
                                                         wirelength_v, wirelength_h,
                                                         &mux_driver_power);
                                                }
                                                max_delay = MAX (max_delay, mux_driver);

                                                selb = selb_delay_tag_path (inrisetime,
                                                                            &outrisetime,
                                                                            &selb_power);
                                                max_delay = MAX (max_delay, selb);

                                                valid_driver = 0;

                                                time_till_compare =
                                                    subbank_address_routing_delay +
                                                    decoder_tag + wordline_tag +
                                                    bitline_tag + sense_amp_tag;

                                                if (fast_cache_access_flag) {
                                                    /*dt: don't wait for the mux signal to travel to the subarray, have the n ways
                                                      routed to the edge of the data array*/
                                                    time_till_select =
                                                        time_till_compare + compare_tag +
                                                        selb;
                                                }
                                                else {
                                                    time_till_select =
                                                        time_till_compare + compare_tag +
                                                        mux_driver +
                                                        selb;
                                                }

                                                if (sequential_access_flag) {
                                                    /*dt: testing sequential access*/
                                                    /*approximating the time it takes the match and valid signal to travel to the edge of the tag array with decode_tag */
                                                    access_time = before_mux + time_till_compare + compare_tag + decoder_tag;
                                                }
                                                else {
                                                    access_time =
                                                        MAX (before_mux,
                                                             time_till_select) +
                                                        after_mux;
                                                }

                                            }

                                            /*
                                             * Calcuate the cycle time
                                             */

                                            precharge_del = precharge_delay(wordline_data);

                                            /*dt: replacing this with a simple calculation:
                                            	  The things which cannot be easily pipelined are:
                                            	  wordline and bitlin
                                            	  senseamp
                                            	  comparator
                                            	  so those are the things which are going to limit cycle time
                                            */
                                            cycle_time = MAX(
                                                             MAX(wordline_data + bitline_data + sense_amp_data,
                                                                 wordline_tag  + bitline_tag  + sense_amp_tag),
                                                             compare_tag);

                                            /*
                                             * The parameters are for a 0.8um process.  A quick way to
                                             * scale the results to another process is to divide all
                                             * the results by circuit_param_t::cparam_FUDGEFACTOR.  Normally, circuit_param_t::cparam_FUDGEFACTOR is 1.
                                             */

                                            //v4.1: All delay and energy components are no longer scaled by circuit_param_t::cparam_FUDGEFACTOR as they have already
                                            //been computed for the input tech node.
                                            allports = parameters->num_readwrite_ports + parameters->num_read_ports + parameters->num_write_ports;
                                            allreadports = parameters->num_readwrite_ports + parameters->num_read_ports;
                                            allwriteports = parameters->num_readwrite_ports + parameters->num_write_ports;

                                            /*dt:We'll add this at the end*/
                                            mult_powerDef(&total_address_routing_power, allports);

                                            reset_powerDef(&total_power);

                                            mac_powerDef(&total_power, &subbank_address_routing_power, allports);
                                            // mac_powerDef(&total_power, &decoder_data_power, allports);
                                            total_power.readOp.dynamic += decoder_data_power.readOp.dynamic * allreadports;
                                            total_power.writeOp.dynamic += decoder_data_power.writeOp.dynamic * allwriteports;

                                            
                                            //mac_powerDef(&total_power, &wordline_data_power, allports);
                                            total_power.readOp.dynamic += wordline_data_power.readOp.dynamic * allreadports;
                                            total_power.writeOp.dynamic += wordline_data_power.writeOp.dynamic * allwriteports;

                                            /*dt: We can have different numbers of read/write ports, so the power numbers have to keep that in mind. Ports which can do both reads and
                                            writes are counted as writes for max power, because writes use full swing and thus use more power. (Assuming full bitline swing is greater 
                                            than senseamp power) */
                                            total_power.readOp.dynamic += bitline_data_power.readOp.dynamic * allreadports;
                                            total_power.writeOp.dynamic += bitline_data_power.writeOp.dynamic * allwriteports;
                                            /*dt: for multiported SRAM cells we assume NAND stacks on all the pure read ports. We assume neglegible
                                            leakage for these ports. The following code adjusts the leakage numbers accordingly.*/
                                            total_power.readOp.leakage += bitline_data_power.readOp.leakage * allreadports;//changed to allreadports by Shyam. Earlier this was allwriteports.
                                            total_power.writeOp.leakage += bitline_data_power.writeOp.leakage * allwriteports;

                                            mac_powerDef(&total_power, &sense_amp_data_power, allreadports);
                                            mac_powerDef(&total_power, &total_out_driver_power, allreadports);
                                            /*dt: Tags are only written to when a cache line is evicted/replaced. The only bit which is written is the dirty bit. The dirty bits are kept
                                            in the data array. */
                                            //if on area_param_t::aparam_pure_sram_flag added by Shyam since tag power should not be added to total power in pure SRAM mode.
                                            if (!area_param_t::aparam_pure_sram_flag) {
                                                mac_powerDef(&total_power, &decoder_tag_power, allreadports);
                                                mac_powerDef(&total_power, &wordline_tag_power, allreadports);
                                                mac_powerDef(&total_power, &bitline_tag_power, allreadports);
                                                mac_powerDef(&total_power, &sense_amp_tag_power, allreadports);
                                                mac_powerDef(&total_power, &compare_tag_power, allreadports);
                                                mac_powerDef(&total_power, &valid_driver_power, allreadports);
                                                mac_powerDef(&total_power, &mux_driver_power, allreadports);
                                                mac_powerDef(&total_power, &selb_power, allreadports);
                                            }
                                            mac_powerDef(&total_power, &data_output_power, allreadports);


                                            reset_powerDef(&total_power_without_routing);
                                            mac_powerDef(&total_power_without_routing, &total_power, 1); // copy over and ..

                                            //total_power.readOp.dynamic /= circuit_param_t::cparam_FUDGEFACTOR;
                                            //total_power.writeOp.dynamic /= circuit_param_t::cparam_FUDGEFACTOR;
                                            /*dt: Leakage isn't scaled with circuit_param_t::cparam_FUDGEFACTOR, because that's already done by the leakage model much more realistically */


                                            mac_powerDef(&total_power_without_routing, &subbank_address_routing_power, -allports); // ... then subtract ..
                                            mac_powerDef(&total_power_without_routing, &total_out_driver_power, -allreadports);
                                            mac_powerDef(&total_power_without_routing, &valid_driver_power, -allreadports);

                                            /*total_power_without_routing.readOp.dynamic *= (*NSubbanks)/ circuit_param_t::cparam_FUDGEFACTOR;
                                            total_power_without_routing.readOp.leakage *= (*NSubbanks);
                                            total_power_without_routing.writeOp.dynamic *= (*NSubbanks)/ circuit_param_t::cparam_FUDGEFACTOR;
                                            total_power_without_routing.writeOp.leakage *= (*NSubbanks);*/

                                            total_power_without_routing.readOp.dynamic *= (*NSubbanks);
                                            total_power_without_routing.readOp.leakage *= (*NSubbanks);
                                            total_power_without_routing.writeOp.dynamic *= (*NSubbanks);
                                            total_power_without_routing.writeOp.leakage *= (*NSubbanks);
                                            /*dt: see above for leakage*/

                                            reset_powerDef(&total_power_allbanks);
                                            //total_power_allbanks.readOp.dynamic = total_power_without_routing.readOp.dynamic + total_address_routing_power.readOp.dynamic / circuit_param_t::cparam_FUDGEFACTOR;
                                            total_power_allbanks.readOp.dynamic = total_power_without_routing.readOp.dynamic + total_address_routing_power.readOp.dynamic;
                                            total_power_allbanks.readOp.leakage = total_power_without_routing.readOp.leakage + total_address_routing_power.readOp.leakage;
                                            //total_power_allbanks.writeOp.dynamic = total_power_without_routing.writeOp.dynamic + total_address_routing_power.writeOp.dynamic / circuit_param_t::cparam_FUDGEFACTOR;
                                            total_power_allbanks.writeOp.dynamic = total_power_without_routing.writeOp.dynamic + total_address_routing_power.writeOp.dynamic;
                                            total_power_allbanks.writeOp.leakage = total_power_without_routing.writeOp.leakage + total_address_routing_power.writeOp.leakage;

                                            //      if (counter==1)
                                            //        fprintf(stderr, "Pow - %f, Acc - %f, Pow - %f, Acc - %f, Combo - %f\n", total_power*1e9, access_time*1e9, total_power/result->max_power, access_time/result->max_access_time, total_power/result->max_power*access_time/result->max_access_time);

                                            if (counter == 1)
                                            {
                                                double temp_readOp_leakage = (total_power.readOp.leakage > 0) ? 
                                                    total_power.readOp.leakage : 0;
                                                double temp_writeOp_leakage = (total_power.writeOp.leakage > 0) ? 
                                                    total_power.writeOp.leakage : 0;
                                                if ( objective_function(1, 1, 1,
                                                                        result->access_time / result->max_access_time,
                                                                        1.0 / arearesult->efficiency,
                                                                        ((result->total_power.readOp.dynamic / result->cycle_time) + temp_readOp_leakage +
                                                                         (result->total_power.writeOp.dynamic / result->cycle_time) + temp_writeOp_leakage) / 2.0
                                                                        / result->max_power)
                                                        >
                                                        objective_function(1, 1, 1,
                                                                           access_time / (result->max_access_time),
                                                                           1.0 / efficiency,
                                                                           ((total_power.readOp.dynamic / (cycle_time)) + temp_readOp_leakage +
                                                                            (total_power.writeOp.dynamic / (cycle_time)) + temp_readOp_leakage) / 2.0
                                                                           / result->max_power)
                                                   )//read and write dynamic energy components changed to power components by Shyam. Earlier
                                                    //read and write dynamic energies were being added to leakage power components and getting swamped
                                                {
                                                    result->senseext_scale = senseext_scale;
                                                    copy_powerDef(&result->total_power, total_power);
                                                    copy_powerDef(&result->total_power_without_routing, total_power_without_routing);
                                                    //copy_and_div_powerDef(&result->total_routing_power,total_address_routing_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    copy_powerDef(&result->total_routing_power, total_address_routing_power);
                                                    copy_powerDef(&result->total_power_allbanks, total_power_allbanks);

                                                    result->subbank_address_routing_delay = subbank_address_routing_delay / circuit_param_t::cparam_FUDGEFACTOR;
                                                    //copy_and_div_powerDef(&result->subbank_address_routing_power,subbank_address_routing_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    copy_powerDef(&result->subbank_address_routing_power, subbank_address_routing_power);

                                                    //result->cycle_time = cycle_time / circuit_param_t::cparam_FUDGEFACTOR;
                                                    //result->access_time = access_time / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->cycle_time = cycle_time;
                                                    result->access_time = access_time;
                                                    result->best_muxover = circuit_param_t::cparam_muxover;
                                                    result->best_Ndwl = Ndwl;
                                                    result->best_Ndbl = Ndbl;
                                                    result->best_Nspd = Nspd;
                                                    result->best_Ntwl = Ntwl;
                                                    result->best_Ntbl = Ntbl;
                                                    result->best_Ntspd = Ntspd;
                                                    /*result->decoder_delay_data = decoder_data / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->decoder_power_data,decoder_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->decoder_delay_tag = decoder_tag / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->decoder_power_tag,decoder_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->dec_tag_driver = decoder_tag_driver / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->dec_tag_3to8 = decoder_tag_3to8 / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->dec_tag_inv = decoder_tag_inv / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->dec_data_driver = decoder_data_driver / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->dec_data_3to8 = decoder_data_3to8 / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->dec_data_inv = decoder_data_inv / circuit_param_t::cparam_FUDGEFACTOR;
                                                    result->wordline_delay_data = wordline_data / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->wordline_power_data,wordline_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->wordline_delay_tag = wordline_tag / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->wordline_power_tag,wordline_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->bitline_delay_data = bitline_data / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->bitline_power_data,bitline_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->bitline_delay_tag = bitline_tag / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->bitline_power_tag,bitline_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->sense_amp_delay_data = sense_amp_data / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->sense_amp_power_data,sense_amp_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->sense_amp_delay_tag = sense_amp_tag / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->sense_amp_power_tag,sense_amp_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->total_out_driver_delay_data = total_out_driver / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->total_out_driver_power_data,total_out_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->compare_part_delay = compare_tag / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->compare_part_power,compare_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->drive_mux_delay = mux_driver / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->drive_mux_power,mux_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->selb_delay = selb / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->selb_power,selb_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->drive_valid_delay = valid_driver / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->drive_valid_power,valid_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->data_output_delay = data_output / circuit_param_t::cparam_FUDGEFACTOR;
                                                    copy_and_div_powerDef(&result->data_output_power,data_output_power,circuit_param_t::cparam_FUDGEFACTOR);
                                                    result->precharge_delay = precharge_del / circuit_param_t::cparam_FUDGEFACTOR;*/

                                                    result->decoder_delay_data = decoder_data;
                                                    copy_powerDef(&result->decoder_power_data, decoder_data_power);
                                                    result->decoder_delay_tag = decoder_tag;
                                                    copy_powerDef(&result->decoder_power_tag, decoder_tag_power);
                                                    result->dec_tag_driver = decoder_tag_driver;
                                                    result->dec_tag_3to8 = decoder_tag_3to8;
                                                    result->dec_tag_inv = decoder_tag_inv;
                                                    result->dec_data_driver = decoder_data_driver;
                                                    result->dec_data_3to8 = decoder_data_3to8;
                                                    result->dec_data_inv = decoder_data_inv;
                                                    result->wordline_delay_data = wordline_data;
                                                    copy_powerDef(&result->wordline_power_data, wordline_data_power);
                                                    result->wordline_delay_tag = wordline_tag;
                                                    copy_powerDef(&result->wordline_power_tag, wordline_tag_power);
                                                    result->bitline_delay_data = bitline_data;
                                                    copy_powerDef(&result->bitline_power_data, bitline_data_power);
                                                    result->bitline_delay_tag = bitline_tag;
                                                    copy_powerDef(&result->bitline_power_tag, bitline_tag_power);
                                                    result->sense_amp_delay_data = sense_amp_data;
                                                    copy_powerDef(&result->sense_amp_power_data, sense_amp_data_power);
                                                    result->sense_amp_delay_tag = sense_amp_tag;
                                                    copy_powerDef(&result->sense_amp_power_tag, sense_amp_tag_power);
                                                    result->total_out_driver_delay_data = total_out_driver;
                                                    copy_powerDef(&result->total_out_driver_power_data, total_out_driver_power);
                                                    result->compare_part_delay = compare_tag;
                                                    copy_powerDef(&result->compare_part_power, compare_tag_power);
                                                    result->drive_mux_delay = mux_driver;
                                                    copy_powerDef(&result->drive_mux_power, mux_driver_power);
                                                    result->selb_delay = selb;
                                                    copy_powerDef(&result->selb_power, selb_power);
                                                    result->drive_valid_delay = valid_driver;
                                                    copy_powerDef(&result->drive_valid_power, valid_driver_power);
                                                    result->data_output_delay = data_output;
                                                    copy_powerDef(&result->data_output_power, data_output_power);
                                                    result->precharge_delay = precharge_del;



                                                    result->data_nor_inputs = data_nor_inputs;
                                                    result->tag_nor_inputs = tag_nor_inputs;
                                                    area_obj->area_subbanked(ADDRESS_BITS,
                                                                             circuit_param_t::cparam_BITOUT,
                                                                             parameters->
                                                                             num_readwrite_ports,
                                                                             parameters->
                                                                             num_read_ports,
                                                                             parameters->
                                                                             num_write_ports,
                                                                             Ndbl, Ndwl,
                                                                             Nspd, Ntbl,
                                                                             Ntwl, Ntspd,
                                                                             *NSubbanks,
                                                                             parameters,
                                                                             arearesult_subbanked,
                                                                             arearesult);


                                                    //v4.1: No longer using calculate_area function as area has already been
                                                    //computed for the given tech node
                                                    /*arearesult->efficiency =
                                                    (*NSubbanks) *
                                                    (area_param_t::aparam_area_all_dataramcells +
                                                    area_param_t::aparam_area_all_tagramcells) * 100 /
                                                    (calculate_area
                                                    (*arearesult_subbanked,
                                                    parameters->fudgefactor) / 100000000.0);*/


                                                    arearesult->efficiency =
                                                        (*NSubbanks) *
                                                        (area_param_t::aparam_area_all_dataramcells +
                                                         area_param_t::aparam_area_all_tagramcells) * 100 /
                                                        (arearesult_subbanked->height * arearesult_subbanked->width / 100000000.0);
                                                    arearesult->aspect_ratio_total =
                                                        (arearesult_subbanked->height / arearesult_subbanked->width);
                                                    arearesult->aspect_ratio_total =
                                                        (arearesult->aspect_ratio_total > 1.0) ?
                                                        (arearesult->aspect_ratio_total) : 1.0 / (arearesult->aspect_ratio_total);
                                                    arearesult->max_efficiency = max_efficiency;
                                                    arearesult->max_aspect_ratio_total = max_aspect_ratio_total;


                                                }
                                            }
                                            else
                                            {
                                                if (result->max_access_time < access_time) {
                                                    result->max_access_time = access_time ;
                                                }
                                                double temp_readOp_leakage = (total_power.readOp.leakage > 0) ? 
                                                    total_power.readOp.leakage : 0;
                                                double temp_writeOp_leakage = (total_power.writeOp.leakage > 0) ? 
                                                    total_power.writeOp.leakage : 0;
                                                
                                                    
                                                if (result->max_power <
                                                        ((total_power.readOp.dynamic / (cycle_time)) + temp_readOp_leakage +
                                                         (total_power.writeOp.dynamic / (cycle_time)) + temp_writeOp_leakage) / 2.0) {
                                                    result->max_power = ((total_power.readOp.dynamic / (cycle_time)) + temp_readOp_leakage +
                                                                         (total_power.writeOp.dynamic / (cycle_time)) + temp_writeOp_leakage) / 2.0;
                                                    //read and write dynamic energy components changed to power components by Shyam. Earlier
                                                    //read and write dynamic energies were being added to leakage power components and getting swamped
                                                }
                                                if (arearesult_temp.max_efficiency < efficiency)
                                                {
                                                    arearesult_temp.max_efficiency = efficiency;
                                                    max_efficiency = efficiency;
                                                }
                                                if (min_efficiency > efficiency) {
                                                    min_efficiency = efficiency;
                                                }
                                                if (max_aspect_ratio_total < aspect_ratio_total_temp) {
                                                    max_aspect_ratio_total = aspect_ratio_total_temp;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* Fully associative model - only vary Ndbl|Ntbl */

            for (Ndbl = 1; Ndbl <= MAXDATAN; Ndbl = Ndbl * 2)
            {
                Ntbl = Ndbl;
                //v4.1: Nspd is now of double data type
                //Ndwl = Nspd = Ntwl = Ntspd = 1;
                Ndwl = Ntwl = Ntspd = 1;
                Nspd = 1.0;


                if (area_obj->data_organizational_parameters_valid(parameters->block_size,
                        1, parameters->cache_size, Ndwl, Ndbl, Nspd, parameters->fully_assoc, (*NSubbanks)))
                {

                    if (8 * parameters->block_size / circuit_param_t::cparam_BITOUT == 1
                            && Nspd == 1)
                    {
                        circuit_param_t::cparam_muxover = 1;
                    }
                    else
                    {
                        if (Nspd > MAX_COL_MUX)
                        {
                            circuit_param_t::cparam_muxover = 8 * parameters->block_size / circuit_param_t::cparam_BITOUT;
                        }
                        else
                        {
                            if (8 * parameters->block_size * Nspd /
                                    circuit_param_t::cparam_BITOUT > MAX_COL_MUX)
                            {
                                circuit_param_t::cparam_muxover =
                                    (8 * parameters->block_size / circuit_param_t::cparam_BITOUT) /
                                    (MAX_COL_MUX / (Nspd));
                            }
                            else
                            {
                                circuit_param_t::cparam_muxover = 1;
                            }
                        }
                    }


                    area_obj->area_subbanked(ADDRESS_BITS, circuit_param_t::cparam_BITOUT,
                                             parameters->num_readwrite_ports,
                                             parameters->num_read_ports,
                                             parameters->num_write_ports, Ndbl, Ndwl,
                                             Nspd, Ntbl, Ntwl, Ntspd, *NSubbanks,
                                             parameters, &arearesult_subbanked_temp,
                                             &arearesult_temp);

                    Subbank_Efficiency =
                        (area_param_t::aparam_area_all_dataramcells +
                         area_param_t::aparam_area_all_tagramcells) * 100 /
                        (arearesult_temp.totalarea / 100000000.0);

                    //v4.1: No longer using calculate_area function as area has already been
                    //computed for the given tech node
                    /*Total_Efficiency =
                      (*NSubbanks) * (area_param_t::aparam_area_all_dataramcells +
                        area_param_t::aparam_area_all_tagramcells) * 100 /
                      (calculate_area (arearesult_subbanked_temp,
                         parameters->fudgefactor) / 100000000.0);*/

                    Total_Efficiency =
                        (*NSubbanks) * (area_param_t::aparam_area_all_dataramcells +
                                        area_param_t::aparam_area_all_tagramcells) * 100 /
                        (arearesult_subbanked_temp.height * arearesult_subbanked_temp.width / 100000000.0);
                    // efficiency = Subbank_Efficiency;
                    efficiency = Total_Efficiency;

                    arearesult_temp.efficiency = efficiency;
                    aspect_ratio_total_temp =
                        (arearesult_subbanked_temp.height /
                         arearesult_subbanked_temp.width);
                    aspect_ratio_total_temp =
                        (aspect_ratio_total_temp >
                         1.0) ? (aspect_ratio_total_temp) : 1.0 /
                        (aspect_ratio_total_temp);

                    arearesult_temp.aspect_ratio_total =
                        aspect_ratio_total_temp;

                    bank_h = 0;
                    bank_v = 0;

                    subbank_dim (parameters->cache_size, parameters->block_size,
                                 parameters->data_associativity,
                                 parameters->fully_assoc, Ndbl, Ndwl, Nspd,
                                 Ntbl, Ntwl, Ntspd, *NSubbanks, &bank_h,
                                 &bank_v);

                    reset_powerDef(&subbank_address_routing_power);
                    /*dt: this has to be reset on every loop iteration, or else we implicitly reuse the risetime from the end
                      of the last loop iteration!*/
                    inrisetime = addr_inrisetime = 0;

                    subbanks_routing_power (parameters->fully_assoc,
                                            parameters->data_associativity,
                                            *NSubbanks, &bank_h, &bank_v,
                                            &total_address_routing_power);

                    if (*NSubbanks > 2)
                    {
                        subbank_address_routing_delay =
                            address_routing_delay (parameters->cache_size,
                                                   parameters->block_size,
                                                   parameters->data_associativity,
                                                   parameters->fully_assoc, Ndwl,
                                                   Ndbl, Nspd, Ntwl, Ntbl, Ntspd,
                                                   NSubbanks, &outrisetime,
                                                   &subbank_address_routing_power);
                    }



                    /* Calculate data side of cache */
                    inrisetime = outrisetime;
                    addr_inrisetime = outrisetime;

                    max_delay = 0;
                    /* tag path contained here */
                    reset_powerDef(&decoder_data_power);
                    decoder_data = fa_tag_delay (parameters->cache_size,
                                                 parameters->block_size,
                                                 Ntwl, Ntbl, Ntspd,
                                                 &tag_delay_part1,
                                                 &tag_delay_part2,
                                                 &tag_delay_part3,
                                                 &tag_delay_part4,
                                                 &tag_delay_part5,
                                                 &tag_delay_part6, &outrisetime,
                                                 &tag_nor_inputs,
                                                 &decoder_data_power);

                    inrisetime = outrisetime;
                    max_delay = MAX (max_delay, decoder_data);
                    Tpre = decoder_data;


                    reset_powerDef(&wordline_data_power);

                    //Added by Shyam to make FA caches work
                    circuit_param_t::cparam_Wdecinvn = 20.0 / circuit_param_t::cparam_FUDGEFACTOR;//From v3.2 #define value
                    circuit_param_t::cparam_Wdecinvp = 40.0 / circuit_param_t::cparam_FUDGEFACTOR;//From v3.2 #define value
                    circuit_param_t::cparam_Woutdrvseln = 24.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrvselp = 40.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrvnandn = 10.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrvnandp = 30.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrvnorn = 5.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrvnorp = 20.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdrivern = 48.0 / circuit_param_t::cparam_FUDGEFACTOR;
                    circuit_param_t::cparam_Woutdriverp = 80.0 / circuit_param_t::cparam_FUDGEFACTOR;

                    colsfa = CHUNKSIZE * parameters->block_size * 1 * Nspd / Ndwl;
                    desiredrisetimefa = circuit_param_t::cparam_krise * log((double)(colsfa)) / 2.0;
                    Clinefa = (bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) + bct->gatecappass(circuit_param_t::cparam_Wmemcella, (circuit_param_t::cparam_BitWidth - 2 * circuit_param_t::cparam_Wmemcella) / 2.0) + circuit_param_t::cparam_Cwordmetal) * colsfa;
                    Rpdrivefa = desiredrisetimefa / (Clinefa * log(VSINV) * -1.0);
                    circuit_param_t::cparam_WwlDrvp = bct->restowidth(Rpdrivefa, PCH);
                    if (circuit_param_t::cparam_WwlDrvp > circuit_param_t::cparam_Wworddrivemax) {
                        circuit_param_t::cparam_WwlDrvp = circuit_param_t::cparam_Wworddrivemax;
                    }
                    //End of Shyam's FA change

                    wordline_data = wordline_delay (
                                        parameters->cache_size,
                                        parameters->block_size,
                                        1, Ndwl, Ndbl, Nspd,
                                        inrisetime, &outrisetime,
                                        &wordline_data_power);

                    inrisetime = outrisetime;
                    max_delay = MAX (max_delay, wordline_data);
                    /*dt: assuming that the precharge delay is equal to decode + wordline delay */
                    Tpre += wordline_data;

                    reset_powerDef(&bitline_data_power);
                    bitline_data = bitline_delay (parameters->cache_size, 1,
                                                  parameters->block_size, Ndwl,
                                                  Ndbl, Nspd, inrisetime,
                                                  &outrisetime,
                                                  &bitline_data_power, Tpre);


                    inrisetime = outrisetime;
                    max_delay = MAX (max_delay, bitline_data);

                    {
                        reset_powerDef(&sense_amp_data_power);
                        sense_amp_data =
                            sense_amp_delay (parameters->cache_size,
                                             parameters->block_size,
                                             parameters->data_associativity,
                                             Ndwl, Ndbl, Nspd,
                                             inrisetime, &outrisetime,
                                             &sense_amp_data_power);

                        max_delay = MAX (max_delay, sense_amp_data);
                    }
                    inrisetime = outrisetime;

                    reset_powerDef(&data_output_power);
                    data_output =
                        dataoutput_delay (parameters->cache_size,
                                          parameters->block_size, 1,
                                          parameters->fully_assoc,
                                          Ndbl, Nspd, Ndwl,
                                          inrisetime, &outrisetime,
                                          &data_output_power);

                    inrisetime = outrisetime;
                    max_delay = MAX (max_delay, data_output);

                    reset_powerDef(&total_out_driver_power);

                    subbank_v = 0;
                    subbank_h = 0;

                    subbank_routing_length (parameters->cache_size,
                                            parameters->block_size,
                                            parameters->data_associativity,
                                            parameters->fully_assoc, Ndbl, Nspd,
                                            Ndwl, Ntbl, Ntwl, Ntspd, *NSubbanks,
                                            &subbank_v, &subbank_h);

                    if (*NSubbanks > 2)
                    {
                        total_out_driver =
                            senseext_driver_delay (parameters->data_associativity,
                                                   parameters->fully_assoc,
                                                   inrisetime, &outrisetime,
                                                   subbank_v, subbank_h,
                                                   &total_out_driver_power);
                    }

                    /*
                                    total_out_driver = total_out_driver_delay(parameters->cache_size,
                    					parameters->block_size,parameters->associativity,parameters->fully_assoc,
                    					Ndbl,Nspd,Ndwl,Ntbl,Ntwl,Ntspd,
                    					*NSubbanks,inrisetime,&outrisetime, &total_out_driver_power);
                    */

                    inrisetime = outrisetime;
                    max_delay = MAX (max_delay, total_out_driver);

                    access_time =
                        subbank_address_routing_delay + decoder_data +
                        wordline_data + bitline_data + sense_amp_data +
                        data_output + total_out_driver;

                    /*
                     * Calcuate the cycle time
                     */

                    precharge_del = precharge_delay(wordline_data);

                    cycle_time = MAX(
                                     MAX(wordline_data + bitline_data + sense_amp_data,
                                         wordline_tag  + bitline_tag + sense_amp_tag),
                                     compare_tag);

                    /*
                     * The parameters are for a 0.8um process.  A quick way to
                     * scale the results to another process is to divide all
                     * the results by circuit_param_t::cparam_FUDGEFACTOR.  Normally, circuit_param_t::cparam_FUDGEFACTOR is 1.
                     */
                    /*dt: see previous comment on sense amp leakage*/
                    /*
                    sense_amp_data_power +=
                    (data_output + total_out_driver) * 500e-6 * 5;
                    */
                    allports = parameters->num_readwrite_ports + parameters->num_read_ports + parameters->num_write_ports;
                    allreadports = parameters->num_readwrite_ports + parameters->num_read_ports;
                    allwriteports = parameters->num_readwrite_ports + parameters->num_write_ports;

                    mult_powerDef(&total_address_routing_power, allports);

                    reset_powerDef(&total_power);

                    mac_powerDef(&total_power, &subbank_address_routing_power, allports);
                    
                    total_power.readOp.dynamic += decoder_data_power.readOp.dynamic * allreadports;
                    total_power.writeOp.dynamic += decoder_data_power.writeOp.dynamic * allwriteports;
                    total_power.readOp.dynamic += wordline_data_power.readOp.dynamic * allreadports;
                    total_power.writeOp.dynamic += wordline_data_power.writeOp.dynamic * allwriteports;


                    /*dt: We can have different numbers of read/write ports, so the power numbers have to keep that in mind. Ports which can do both reads and
                    writes are counted as writes for max power, because writes use full swing and thus use more power. (Assuming full bitline swing is greater 
                    than senseamp power) */
                    total_power.readOp.dynamic += bitline_data_power.readOp.dynamic * allreadports;
                    total_power.writeOp.dynamic += bitline_data_power.writeOp.dynamic * allwriteports;
                    /*dt: for multiported SRAM cells we assume NAND stacks on all the pure read ports. We assume neglegible
                    leakage for these ports. The following code adjusts the leakage numbers accordingly.*/
                    total_power.readOp.leakage += bitline_data_power.readOp.leakage * allwriteports;
                    total_power.writeOp.leakage += bitline_data_power.writeOp.leakage * allwriteports;

                    mac_powerDef(&total_power, &sense_amp_data_power, allreadports);
                    mac_powerDef(&total_power, &total_out_driver_power, allreadports);
                    /*dt: in a fully associative cache we don't have tag decoders, wordlines or bitlines, etc. . Only CAM */
                    reset_powerDef(&decoder_tag_power);
                    reset_powerDef(&wordline_tag_power);
                    reset_powerDef(&bitline_tag_power);
                    reset_powerDef(&sense_amp_tag_power);
                    reset_powerDef(&compare_tag_power);
                    reset_powerDef(&valid_driver_power);
                    reset_powerDef(&mux_driver_power);
                    reset_powerDef(&selb_power);

                    mac_powerDef(&total_power, &data_output_power, allreadports);

                    reset_powerDef(&total_power_without_routing);
                    mac_powerDef(&total_power_without_routing, &total_power, 1); // copy over and ..

                    //total_power.readOp.dynamic /= circuit_param_t::cparam_FUDGEFACTOR;
                    //total_power.writeOp.dynamic /= circuit_param_t::cparam_FUDGEFACTOR;
                    /*dt: Leakage isn't scaled with circuit_param_t::cparam_FUDGEFACTOR, because that's already done by the leakage model much more realistically */

                    mac_powerDef(&total_power_without_routing, &subbank_address_routing_power, -allports); // ... then subtract ..
                    mac_powerDef(&total_power_without_routing, &total_out_driver_power, -allreadports);
                    mac_powerDef(&total_power_without_routing, &valid_driver_power, -allreadports);

                    /*dt: See above for leakage */
                    //total_power_without_routing.readOp.dynamic *= (*NSubbanks)/ circuit_param_t::cparam_FUDGEFACTOR;
                    total_power_without_routing.readOp.dynamic *= (*NSubbanks);
                    total_power_without_routing.readOp.leakage *= (*NSubbanks);
                    //total_power_without_routing.writeOp.dynamic *= (*NSubbanks)/ circuit_param_t::cparam_FUDGEFACTOR;
                    total_power_without_routing.writeOp.dynamic *= (*NSubbanks);
                    total_power_without_routing.writeOp.leakage *= (*NSubbanks);

                    //total_power_allbanks.readOp.dynamic = total_power_without_routing.readOp.dynamic + total_address_routing_power.readOp.dynamic / circuit_param_t::cparam_FUDGEFACTOR;
                    total_power_allbanks.readOp.dynamic = total_power_without_routing.readOp.dynamic + total_address_routing_power.readOp.dynamic ;
                    total_power_allbanks.readOp.leakage = total_power_without_routing.readOp.leakage + total_address_routing_power.readOp.leakage;
                    //total_power_allbanks.writeOp.dynamic = total_power_without_routing.writeOp.dynamic + total_address_routing_power.writeOp.dynamic / circuit_param_t::cparam_FUDGEFACTOR;
                    total_power_allbanks.writeOp.dynamic = total_power_without_routing.writeOp.dynamic + total_address_routing_power.writeOp.dynamic;
                    total_power_allbanks.writeOp.leakage = total_power_without_routing.writeOp.leakage + total_address_routing_power.writeOp.leakage;

                    if (counter == 1)
                    {
                        // if ((result->total_power/result->max_power)/2+(result->access_time/result->max_access_time) > ((total_power/result->max_power)/2+access_time/(result->max_access_time*circuit_param_t::cparam_FUDGEFACTOR))) {

                        if ( objective_function(1, 1, 1,
                                                result->access_time / result->max_access_time,
                                                1.0 / arearesult->efficiency,
                                                ((result->total_power.readOp.dynamic / result->cycle_time) + result->total_power.readOp.leakage +
                                                 (result->total_power.writeOp.dynamic / result->cycle_time) + result->total_power.writeOp.leakage) / 2.0
                                                / result->max_power)
                                >
                                objective_function(1, 1, 1,
                                                   access_time / (result->max_access_time),
                                                   1.0 / efficiency,
                                                   ((total_power.readOp.dynamic / (cycle_time)) + total_power.readOp.leakage +
                                                    (total_power.writeOp.dynamic / (cycle_time)) + total_power.writeOp.leakage) / 2.0
                                                   / result->max_power)
                           )//read and write dynamic energy components changed to power components by Shyam. Earlier
                            //read and write dynamic energies were being added to leakage power components and getting swamped

                        {
                            // if ((result->total_power/result->max_power)/2+(result->access_time/result->max_access_time) + (min_efficiency/arearesult->efficiency)/4 + (arearesult->aspect_ratio_total/max_aspect_ratio_total)/3 > ((total_power/result->max_power)/2+access_time/(result->max_access_time*circuit_param_t::cparam_FUDGEFACTOR)+ (min_efficiency/efficiency)/4 + (arearesult_temp.aspect_ratio_total/max_aspect_ratio_total)/3)) {

                            //          if (result->cycle_time+1e-11*(result->best_Ndwl+result->best_Ndbl+result->best_Nspd+result->best_Ntwl+result->best_Ntbl+result->best_Ntspd) > cycle_time/circuit_param_t::cparam_FUDGEFACTOR+1e-11*(Ndwl+Ndbl+Nspd+Ntwl+Ntbl+Ntspd)) {


                            result->senseext_scale = senseext_scale;
                            copy_powerDef(&result->total_power, total_power);
                            copy_powerDef(&result->total_power_without_routing, total_power_without_routing);
                            //copy_and_div_powerDef(&result->total_routing_power,total_address_routing_power,circuit_param_t::cparam_FUDGEFACTOR);
                            copy_powerDef(&result->total_routing_power, total_address_routing_power);
                            copy_powerDef(&result->total_power_allbanks, total_power_allbanks);

                            /*result->subbank_address_routing_delay = subbank_address_routing_delay / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->subbank_address_routing_power,subbank_address_routing_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->cycle_time = cycle_time / circuit_param_t::cparam_FUDGEFACTOR;
                            result->access_time = access_time / circuit_param_t::cparam_FUDGEFACTOR;
                            result->best_Ndwl = Ndwl;
                            result->best_Ndbl = Ndbl;
                            result->best_Nspd = Nspd;
                            result->best_Ntwl = Ntwl;
                            result->best_Ntbl = Ntbl;
                            result->best_Ntspd = Ntspd;
                            result->decoder_delay_data = decoder_data / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->decoder_power_data,decoder_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->decoder_delay_tag = decoder_tag / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->decoder_power_tag,decoder_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->dec_tag_driver = decoder_tag_driver / circuit_param_t::cparam_FUDGEFACTOR;
                            result->dec_tag_3to8 = decoder_tag_3to8 / circuit_param_t::cparam_FUDGEFACTOR;
                            result->dec_tag_inv = decoder_tag_inv / circuit_param_t::cparam_FUDGEFACTOR;
                            result->dec_data_driver = decoder_data_driver / circuit_param_t::cparam_FUDGEFACTOR;
                            result->dec_data_3to8 = decoder_data_3to8 / circuit_param_t::cparam_FUDGEFACTOR;
                            result->dec_data_inv = decoder_data_inv / circuit_param_t::cparam_FUDGEFACTOR;
                            result->wordline_delay_data = wordline_data / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->wordline_power_data,wordline_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->wordline_delay_tag = wordline_tag / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->wordline_power_tag,wordline_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->bitline_delay_data = bitline_data / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->bitline_power_data,bitline_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->bitline_delay_tag = bitline_tag / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->bitline_power_tag,bitline_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->sense_amp_delay_data = sense_amp_data / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->sense_amp_power_data,sense_amp_data_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->sense_amp_delay_tag = sense_amp_tag / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->sense_amp_power_tag,sense_amp_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->total_out_driver_delay_data = total_out_driver / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->total_out_driver_power_data,total_out_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->compare_part_delay = compare_tag / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->compare_part_power,compare_tag_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->drive_mux_delay = mux_driver / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->drive_mux_power,mux_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->selb_delay = selb / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->selb_power,selb_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->drive_valid_delay = valid_driver / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->drive_valid_power,valid_driver_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->data_output_delay = data_output / circuit_param_t::cparam_FUDGEFACTOR;
                            copy_and_div_powerDef(&result->data_output_power,data_output_power,circuit_param_t::cparam_FUDGEFACTOR);
                            result->precharge_delay = precharge_del / circuit_param_t::cparam_FUDGEFACTOR;*/

                            result->subbank_address_routing_delay = subbank_address_routing_delay;
                            copy_powerDef(&result->subbank_address_routing_power, subbank_address_routing_power);
                            result->cycle_time = cycle_time;
                            result->access_time = access_time;
                            result->best_Ndwl = Ndwl;
                            result->best_Ndbl = Ndbl;
                            result->best_Nspd = Nspd;
                            result->best_Ntwl = Ntwl;
                            result->best_Ntbl = Ntbl;
                            result->best_Ntspd = Ntspd;
                            result->decoder_delay_data = decoder_data;
                            copy_powerDef(&result->decoder_power_data, decoder_data_power);
                            result->decoder_delay_tag = decoder_tag;
                            copy_powerDef(&result->decoder_power_tag, decoder_tag_power);
                            result->dec_tag_driver = decoder_tag_driver;
                            result->dec_tag_3to8 = decoder_tag_3to8;
                            result->dec_tag_inv = decoder_tag_inv;
                            result->dec_data_driver = decoder_data_driver;
                            result->dec_data_3to8 = decoder_data_3to8;
                            result->dec_data_inv = decoder_data_inv;
                            result->wordline_delay_data = wordline_data;
                            copy_powerDef(&result->wordline_power_data, wordline_data_power);
                            result->wordline_delay_tag = wordline_tag;
                            copy_powerDef(&result->wordline_power_tag, wordline_tag_power);
                            result->bitline_delay_data = bitline_data;
                            copy_powerDef(&result->bitline_power_data, bitline_data_power);
                            result->bitline_delay_tag = bitline_tag;
                            copy_powerDef(&result->bitline_power_tag, bitline_tag_power);
                            result->sense_amp_delay_data = sense_amp_data;
                            copy_powerDef(&result->sense_amp_power_data, sense_amp_data_power);
                            result->sense_amp_delay_tag = sense_amp_tag;
                            copy_powerDef(&result->sense_amp_power_tag, sense_amp_tag_power);
                            result->total_out_driver_delay_data = total_out_driver;
                            copy_powerDef(&result->total_out_driver_power_data, total_out_driver_power);
                            result->compare_part_delay = compare_tag;
                            copy_powerDef(&result->compare_part_power, compare_tag_power);
                            result->drive_mux_delay = mux_driver;
                            copy_powerDef(&result->drive_mux_power, mux_driver_power);
                            result->selb_delay = selb;
                            copy_powerDef(&result->selb_power, selb_power);
                            result->drive_valid_delay = valid_driver;
                            copy_powerDef(&result->drive_valid_power, valid_driver_power);
                            result->data_output_delay = data_output;
                            copy_powerDef(&result->data_output_power, data_output_power);
                            result->precharge_delay = precharge_del;
                            result->data_nor_inputs = data_nor_inputs;
                            result->tag_nor_inputs = tag_nor_inputs;

                            area_obj->area_subbanked (ADDRESS_BITS, circuit_param_t::cparam_BITOUT,
                                                      parameters->num_readwrite_ports,
                                                      parameters->num_read_ports,
                                                      parameters->num_write_ports, Ndbl,
                                                      Ndwl, Nspd, Ntbl, Ntwl, Ntspd,
                                                      *NSubbanks, parameters,
                                                      arearesult_subbanked, arearesult);

                            //v4.1: No longer using calculate_area function as area has already been
                            //computed for the given tech node
                            /*arearesult->efficiency =
                              (*NSubbanks) * (area_param_t::aparam_area_all_dataramcells +
                                area_param_t::aparam_area_all_tagramcells) * 100 /
                              (calculate_area (*arearesult_subbanked,
                                 parameters->fudgefactor) /
                               100000000.0);*/

                            arearesult->efficiency =
                                (*NSubbanks) * (area_param_t::aparam_area_all_dataramcells +
                                                area_param_t::aparam_area_all_tagramcells) * 100 /
                                (arearesult_subbanked->height * arearesult_subbanked->width / 100000000.0);

                            arearesult->aspect_ratio_total =
                                (arearesult_subbanked->height /
                                 arearesult_subbanked->width);
                            arearesult->aspect_ratio_total =
                                (arearesult->aspect_ratio_total >
                                 1.0) ? (arearesult->aspect_ratio_total) : 1.0 /
                                (arearesult->aspect_ratio_total);
                            arearesult->max_efficiency = max_efficiency;
                            arearesult->max_aspect_ratio_total =
                                max_aspect_ratio_total;

                        }
                    }
                    else
                    {

                        if (result->max_access_time < access_time)
                            result->max_access_time = access_time;
                        if (result->max_power <
                                ((total_power.readOp.dynamic / (cycle_time)) + total_power.readOp.leakage +
                                 (total_power.writeOp.dynamic / (cycle_time)) + total_power.writeOp.leakage) / 2.0) {
                            result->max_power = ((total_power.readOp.dynamic / (cycle_time)) + total_power.readOp.leakage +
                                                 (total_power.writeOp.dynamic / (cycle_time)) + total_power.writeOp.leakage) / 2.0;
                        } //read and write dynamic energy components changed to power components by Shyam. Earlier
                        //read and write dynamic energies were being added to leakage power components and getting swamped
                        if (arearesult_temp.max_efficiency < efficiency)
                        {
                            arearesult_temp.max_efficiency = efficiency;
                            max_efficiency = efficiency;
                        }
                        if (min_efficiency > efficiency)
                        {
                            min_efficiency = efficiency;
                        }
                        if (max_aspect_ratio_total < aspect_ratio_total_temp) {
                            max_aspect_ratio_total = aspect_ratio_total_temp;
                        }

                    }
                }

            }
        }
    }
}

cacti_time_t::cacti_time_t(double tech, leakage_t *_leak, basic_circuit_t *_bct)
: leakage_obj(_leak), bct(_bct)
{
    
    Vt_bit_nmos_low = leakage_obj->get_Vt_bit_nmos_low();
    Vthn = leakage_obj->get_Vthn();
    Vt_bit_pmos_low = leakage_obj->get_Vt_bit_pmos_low();
    Vthp = leakage_obj->get_Vthp();
    Tkelvin = 300.0;
    // TODO assign proper values
    // Tox;
    // Vt_bit_nmos_high, Vt_bit_pmos_high;
}



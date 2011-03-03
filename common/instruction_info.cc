/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: instruction_info.cc,v 1.1.2.2 2006/09/28 15:18:29 pwells Exp $");

#include <sstream>
#include "definitions.h"
#include "mai_instr.h"
#include "fu.h"
#include "isa.h"
#include "instruction_info.h"
#include "dynamic.h"


inline int64
sign_ext64 (int64 w, int high_bit) {
//	return w;

	int msb = (w >> high_bit) & 0x1;
	int64 result = 0;

	if (msb) 
		result = ((~result << (high_bit + 1)) | w);
	else 
		result = w;
	
	return result;
}

uint32 inst_info_t::maskBits32 (uint32 data, int start, int stop)
{
	int big;
	int small;
	int mask;

	if (stop > start) {
    	big = stop;
    	small = start;
	} else {
		big = start;
		small = stop;
  	}
  
	if (big >= 31) {
    	mask = ~0;
  	} else {
    	mask = ~(~0 << (big + 1));
  	}
	
	return ((data & mask) >> small);
}

branch_type_t inst_info_t::get_branch_type()
{
    return branch_type;
}

instruction_type_t inst_info_t::get_instruction_type()
{
    return type;
}
 
addr_t inst_info_t::get_target()
{
    return target;
}

void inst_info_t::clear()
{
    type = IT_TYPES;
    branch_type = BRANCH_TYPES;
    for (int32 k = 0; k < RI_NUMBER; k ++) 
		l_reg [k] = REG_NA;
	num_srcs = num_dests = 0;
    offset = 0;
	annul = false;
    opcode = i_opcodes;
	for (int32 k = 0; k < RN_NUMBER; k ++) {
		reg_sz [k] = 1;  // single-word register
		reg_fp [k] = false;
	}
    rd_reg = 0xfff;
    taken = false;
}

inst_info_t::inst_info_t()
{
    clear();
    pc_reg = SIM_get_register_number(SIM_current_processor(), "pc");
	
    opcode_names[i_add] = "i_add";
    opcode_names[i_addcc] = "i_addcc";
    opcode_names[i_addc] = "i_addc";
    opcode_names[i_addccc] = "i_addccc";
    opcode_names[i_and] = "i_and";
    opcode_names[i_andcc] = "i_andcc";
    opcode_names[i_andn] = "i_andn";
    opcode_names[i_andncc] = "i_andncc";
    opcode_names[i_fba] = "i_fba";
    opcode_names[i_ba] = "i_ba";
    opcode_names[i_fbpa] = "i_fbpa";
    opcode_names[i_bpa] = "i_bpa";
    opcode_names[i_fbn] = "i_fbn";
    opcode_names[i_fbpn] = "i_fbpn";
    opcode_names[i_bn] = "i_bn";
    opcode_names[i_bpn] = "i_bpn";
    opcode_names[i_bpne] = "i_bpne";
    opcode_names[i_bpe] = "i_bpe";
    opcode_names[i_bpg] = "i_bpg";
    opcode_names[i_bple] = "i_bple";
    opcode_names[i_bpge] = "i_bpge";
    opcode_names[i_bpl] = "i_bpl";
    opcode_names[i_bpgu] = "i_bpgu";
    opcode_names[i_bpleu] = "i_bpleu";
    opcode_names[i_bpcc] = "i_bpcc";
    opcode_names[i_bpcs] = "i_bpcs";
    opcode_names[i_bppos] = "i_bppos";
    opcode_names[i_bpneg] = "i_bpneg";
    opcode_names[i_bpvc] = "i_bpvc";
    opcode_names[i_bpvs] = "i_bpvs";
    opcode_names[i_call] = "i_call";
    opcode_names[i_casa] = "i_casa";
    opcode_names[i_casxa] = "i_casxa";
    opcode_names[i_done] = "i_done";
    opcode_names[i_retry] = "i_retry";
    opcode_names[i_jmpl] = "i_jmpl";
    opcode_names[i_fabss] = "i_fabss";
    opcode_names[i_fabsd] = "i_fabsd";
    opcode_names[i_fabsq] = "i_fabsq";
    opcode_names[i_fadds] = "i_fadds";
    opcode_names[i_faddd] = "i_faddd";
    opcode_names[i_faddq] = "i_faddq";
    opcode_names[i_fsubs] = "i_fsubs";
    opcode_names[i_fsubd] = "i_fsubd";
    opcode_names[i_fsubq] = "i_fsubq";
    opcode_names[i_fcmps] = "i_fcmps";
    opcode_names[i_fcmpd] = "i_fcmpd";
    opcode_names[i_fcmpq] = "i_fcmpq";
    opcode_names[i_fcmpes] = "i_fcmpes";
    opcode_names[i_fcmped] = "i_fcmped";
    opcode_names[i_fcmpeq] = "i_fcmpeq";
    opcode_names[i_fmovs] = "i_fmovs";
    opcode_names[i_fmovd] = "i_fmovd";
    opcode_names[i_fmovq] = "i_fmovq";
    opcode_names[i_fnegs] = "i_fnegs";
    opcode_names[i_fnegd] = "i_fnegd";
    opcode_names[i_fnegq] = "i_fnegq";
    opcode_names[i_fmuls] = "i_fmuls";
    opcode_names[i_fmuld] = "i_fmuld";
    opcode_names[i_fmulq] = "i_fmulq";
    opcode_names[i_fdivs] = "i_fdivs";
    opcode_names[i_fdivd] = "i_fdivd";
    opcode_names[i_fdivq] = "i_fdivq";
    opcode_names[i_fsmuld] = "i_fsmuld";
    opcode_names[i_fdmulq] = "i_fdmulq";
    opcode_names[i_fsqrts] = "i_fsqrts";
    opcode_names[i_fsqrtd] = "i_fsqrtd";
    opcode_names[i_fsqrtq] = "i_fsqrtq";
    opcode_names[i_retrn] = "i_retrn";
    opcode_names[i_brz] = "i_brz";
    opcode_names[i_brlez] = "i_brlez";
    opcode_names[i_brlz] = "i_brlz";
    opcode_names[i_brnz] = "i_brnz";
    opcode_names[i_brgz] = "i_brgz";
    opcode_names[i_brgez] = "i_brgez";
    opcode_names[i_fbu] = "i_fbu";
    opcode_names[i_fbg] = "i_fbg";
    opcode_names[i_fbug] = "i_fbug";
    opcode_names[i_fbl] = "i_fbl";
    opcode_names[i_fbul] = "i_fbul";
    opcode_names[i_fblg] = "i_fblg";
    opcode_names[i_fbne] = "i_fbne";
    opcode_names[i_fbe] = "i_fbe";
    opcode_names[i_fbue] = "i_fbue";
    opcode_names[i_fbge] = "i_fbge";
    opcode_names[i_fbuge] = "i_fbuge";
    opcode_names[i_fble] = "i_fble";
    opcode_names[i_fbule] = "i_fbule";
    opcode_names[i_fbo] = "i_fbo";
    opcode_names[i_fbpu] = "i_fbpu";
    opcode_names[i_fbpg] = "i_fbpg";
    opcode_names[i_fbpug] = "i_fbpug";
    opcode_names[i_fbpl] = "i_fbpl";
    opcode_names[i_fbpul] = "i_fbpul";
    opcode_names[i_fbplg] = "i_fbplg";
    opcode_names[i_fbpne] = "i_fbpne";
    opcode_names[i_fbpe] = "i_fbpe";
    opcode_names[i_fbpue] = "i_fbpue";
    opcode_names[i_fbpge] = "i_fbpge";
    opcode_names[i_fbpuge] = "i_fbpuge";
    opcode_names[i_fbple] = "i_fbple";
    opcode_names[i_fbpule] = "i_fbpule";
    opcode_names[i_fbpo] = "i_fbpo";
    opcode_names[i_bne] = "i_bne";
    opcode_names[i_be] = "i_be";
    opcode_names[i_bg] = "i_bg";
    opcode_names[i_ble] = "i_ble";
    opcode_names[i_bge] = "i_bge";
    opcode_names[i_bl] = "i_bl";
    opcode_names[i_bgu] = "i_bgu";
    opcode_names[i_bleu] = "i_bleu";
    opcode_names[i_bcc] = "i_bcc";
    opcode_names[i_bcs] = "i_bcs";
    opcode_names[i_bpos] = "i_bpos";
    opcode_names[i_bneg] = "i_bneg";
    opcode_names[i_bvc] = "i_bvc";
    opcode_names[i_bvs] = "i_bvs";
    opcode_names[i_fstox] = "i_fstox";
    opcode_names[i_fdtox] = "i_fdtox";
    opcode_names[i_fqtox] = "i_fqtox";
    opcode_names[i_fstoi] = "i_fstoi";
    opcode_names[i_fdtoi] = "i_fdtoi";
    opcode_names[i_fqtoi] = "i_fqtoi";
    opcode_names[i_fstod] = "i_fstod";
    opcode_names[i_fstoq] = "i_fstoq";
    opcode_names[i_fdtos] = "i_fdtos";
    opcode_names[i_fdtoq] = "i_fdtoq";
    opcode_names[i_fqtos] = "i_fqtos";
    opcode_names[i_fqtod] = "i_fqtod";
    opcode_names[i_fxtos] = "i_fxtos";
    opcode_names[i_fxtod] = "i_fxtod";
    opcode_names[i_fxtoq] = "i_fxtoq";
    opcode_names[i_fitos] = "i_fitos";
    opcode_names[i_fitod] = "i_fitod";
    opcode_names[i_fitoq] = "i_fitoq";
    opcode_names[i_fmovfsn] = "i_fmovfsn";
    opcode_names[i_fmovfsne] = "i_fmovfsne";
    opcode_names[i_fmovfslg] = "i_fmovfslg";
    opcode_names[i_fmovfsul] = "i_fmovfsul";
    opcode_names[i_fmovfsl] = "i_fmovfsl";
    opcode_names[i_fmovfsug] = "i_fmovfsug";
    opcode_names[i_fmovfsg] = "i_fmovfsg";
    opcode_names[i_fmovfsu] = "i_fmovfsu";
    opcode_names[i_fmovfsa] = "i_fmovfsa";
    opcode_names[i_fmovfse] = "i_fmovfse";
    opcode_names[i_fmovfsue] = "i_fmovfsue";
    opcode_names[i_fmovfsge] = "i_fmovfsge";
    opcode_names[i_fmovfsuge] = "i_fmovfsuge";
    opcode_names[i_fmovfsle] = "i_fmovfsle";
    opcode_names[i_fmovfsule] = "i_fmovfsule";
    opcode_names[i_fmovfso] = "i_fmovfso";
    opcode_names[i_fmovfdn] = "i_fmovfdn";
    opcode_names[i_fmovfdne] = "i_fmovfdne";
    opcode_names[i_fmovfdlg] = "i_fmovfdlg";
    opcode_names[i_fmovfdul] = "i_fmovfdul";
    opcode_names[i_fmovfdl] = "i_fmovfdl";
    opcode_names[i_fmovfdug] = "i_fmovfdug";
    opcode_names[i_fmovfdg] = "i_fmovfdg";
    opcode_names[i_fmovfdu] = "i_fmovfdu";
    opcode_names[i_fmovfda] = "i_fmovfda";
    opcode_names[i_fmovfde] = "i_fmovfde";
    opcode_names[i_fmovfdue] = "i_fmovfdue";
    opcode_names[i_fmovfdge] = "i_fmovfdge";
    opcode_names[i_fmovfduge] = "i_fmovfduge";
    opcode_names[i_fmovfdle] = "i_fmovfdle";
    opcode_names[i_fmovfdule] = "i_fmovfdule";
    opcode_names[i_fmovfdo] = "i_fmovfdo";
    opcode_names[i_fmovfqn] = "i_fmovfqn";
    opcode_names[i_fmovfqne] = "i_fmovfqne";
    opcode_names[i_fmovfqlg] = "i_fmovfqlg";
    opcode_names[i_fmovfqul] = "i_fmovfqul";
    opcode_names[i_fmovfql] = "i_fmovfql";
    opcode_names[i_fmovfqug] = "i_fmovfqug";
    opcode_names[i_fmovfqg] = "i_fmovfqg";
    opcode_names[i_fmovfqu] = "i_fmovfqu";
    opcode_names[i_fmovfqa] = "i_fmovfqa";
    opcode_names[i_fmovfqe] = "i_fmovfqe";
    opcode_names[i_fmovfque] = "i_fmovfque";
    opcode_names[i_fmovfqge] = "i_fmovfqge";
    opcode_names[i_fmovfquge] = "i_fmovfquge";
    opcode_names[i_fmovfqle] = "i_fmovfqle";
    opcode_names[i_fmovfqule] = "i_fmovfqule";
    opcode_names[i_fmovfqo] = "i_fmovfqo";
    opcode_names[i_fmovsn] = "i_fmovsn";
    opcode_names[i_fmovse] = "i_fmovse";
    opcode_names[i_fmovsle] = "i_fmovsle";
    opcode_names[i_fmovsl] = "i_fmovsl";
    opcode_names[i_fmovsleu] = "i_fmovsleu";
    opcode_names[i_fmovscs] = "i_fmovscs";
    opcode_names[i_fmovsneg] = "i_fmovsneg";
    opcode_names[i_fmovsvs] = "i_fmovsvs";
    opcode_names[i_fmovsa] = "i_fmovsa";
    opcode_names[i_fmovsne] = "i_fmovsne";
    opcode_names[i_fmovsg] = "i_fmovsg";
    opcode_names[i_fmovsge] = "i_fmovsge";
    opcode_names[i_fmovsgu] = "i_fmovsgu";
    opcode_names[i_fmovscc] = "i_fmovscc";
    opcode_names[i_fmovspos] = "i_fmovspos";
    opcode_names[i_fmovsvc] = "i_fmovsvc";
    opcode_names[i_fmovdn] = "i_fmovdn";
    opcode_names[i_fmovde] = "i_fmovde";
    opcode_names[i_fmovdle] = "i_fmovdle";
    opcode_names[i_fmovdl] = "i_fmovdl";
    opcode_names[i_fmovdleu] = "i_fmovdleu";
    opcode_names[i_fmovdcs] = "i_fmovdcs";
    opcode_names[i_fmovdneg] = "i_fmovdneg";
    opcode_names[i_fmovdvs] = "i_fmovdvs";
    opcode_names[i_fmovda] = "i_fmovda";
    opcode_names[i_fmovdne] = "i_fmovdne";
    opcode_names[i_fmovdg] = "i_fmovdg";
    opcode_names[i_fmovdge] = "i_fmovdge";
    opcode_names[i_fmovdgu] = "i_fmovdgu";
    opcode_names[i_fmovdcc] = "i_fmovdcc";
    opcode_names[i_fmovdpos] = "i_fmovdpos";
    opcode_names[i_fmovdvc] = "i_fmovdvc";
    opcode_names[i_fmovqn] = "i_fmovqn";
    opcode_names[i_fmovqe] = "i_fmovqe";
    opcode_names[i_fmovqle] = "i_fmovqle";
    opcode_names[i_fmovql] = "i_fmovql";
    opcode_names[i_fmovqleu] = "i_fmovqleu";
    opcode_names[i_fmovqcs] = "i_fmovqcs";
    opcode_names[i_fmovqneg] = "i_fmovqneg";
    opcode_names[i_fmovqvs] = "i_fmovqvs";
    opcode_names[i_fmovqa] = "i_fmovqa";
    opcode_names[i_fmovqne] = "i_fmovqne";
    opcode_names[i_fmovqg] = "i_fmovqg";
    opcode_names[i_fmovqge] = "i_fmovqge";
    opcode_names[i_fmovqgu] = "i_fmovqgu";
    opcode_names[i_fmovqcc] = "i_fmovqcc";
    opcode_names[i_fmovqpos] = "i_fmovqpos";
    opcode_names[i_fmovqvc] = "i_fmovqvc";
    opcode_names[i_fmovrsz] = "i_fmovrsz";
    opcode_names[i_fmovrslez] = "i_fmovrslez";
    opcode_names[i_fmovrslz] = "i_fmovrslz";
    opcode_names[i_fmovrsnz] = "i_fmovrsnz";
    opcode_names[i_fmovrsgz] = "i_fmovrsgz";
    opcode_names[i_fmovrsgez] = "i_fmovrsgez";
    opcode_names[i_fmovrdz] = "i_fmovrdz";
    opcode_names[i_fmovrdlez] = "i_fmovrdlez";
    opcode_names[i_fmovrdlz] = "i_fmovrdlz";
    opcode_names[i_fmovrdnz] = "i_fmovrdnz";
    opcode_names[i_fmovrdgz] = "i_fmovrdgz";
    opcode_names[i_fmovrdgez] = "i_fmovrdgez";
    opcode_names[i_fmovrqz] = "i_fmovrqz";
    opcode_names[i_fmovrqlez] = "i_fmovrqlez";
    opcode_names[i_fmovrqlz] = "i_fmovrqlz";
    opcode_names[i_fmovrqnz] = "i_fmovrqnz";
    opcode_names[i_fmovrqgz] = "i_fmovrqgz";
    opcode_names[i_fmovrqgez] = "i_fmovrqgez";
    opcode_names[i_mova] = "i_mova";
    opcode_names[i_movfa] = "i_movfa";
    opcode_names[i_movn] = "i_movn";
    opcode_names[i_movfn] = "i_movfn";
    opcode_names[i_movne] = "i_movne";
    opcode_names[i_move] = "i_move";
    opcode_names[i_movg] = "i_movg";
    opcode_names[i_movle] = "i_movle";
    opcode_names[i_movge] = "i_movge";
    opcode_names[i_movl] = "i_movl";
    opcode_names[i_movgu] = "i_movgu";
    opcode_names[i_movleu] = "i_movleu";
    opcode_names[i_movcc] = "i_movcc";
    opcode_names[i_movcs] = "i_movcs";
    opcode_names[i_movpos] = "i_movpos";
    opcode_names[i_movneg] = "i_movneg";
    opcode_names[i_movvc] = "i_movvc";
    opcode_names[i_movvs] = "i_movvs";
    opcode_names[i_movfu] = "i_movfu";
    opcode_names[i_movfg] = "i_movfg";
    opcode_names[i_movfug] = "i_movfug";
    opcode_names[i_movfl] = "i_movfl";
    opcode_names[i_movful] = "i_movful";
    opcode_names[i_movflg] = "i_movflg";
    opcode_names[i_movfne] = "i_movfne";
    opcode_names[i_movfe] = "i_movfe";
    opcode_names[i_movfue] = "i_movfue";
    opcode_names[i_movfge] = "i_movfge";
    opcode_names[i_movfuge] = "i_movfuge";
    opcode_names[i_movfle] = "i_movfle";
    opcode_names[i_movfule] = "i_movfule";
    opcode_names[i_movfo] = "i_movfo";
    opcode_names[i_movrz] = "i_movrz";
    opcode_names[i_movrlez] = "i_movrlez";
    opcode_names[i_movrlz] = "i_movrlz";
    opcode_names[i_movrnz] = "i_movrnz";
    opcode_names[i_movrgz] = "i_movrgz";
    opcode_names[i_movrgez] = "i_movrgez";
    opcode_names[i_ta] = "i_ta";
    opcode_names[i_tn] = "i_tn";
    opcode_names[i_tne] = "i_tne";
    opcode_names[i_te] = "i_te";
    opcode_names[i_tg] = "i_tg";
    opcode_names[i_tle] = "i_tle";
    opcode_names[i_tge] = "i_tge";
    opcode_names[i_tl] = "i_tl";
    opcode_names[i_tgu] = "i_tgu";
    opcode_names[i_tleu] = "i_tleu";
    opcode_names[i_tcc] = "i_tcc";
    opcode_names[i_tcs] = "i_tcs";
    opcode_names[i_tpos] = "i_tpos";
    opcode_names[i_tneg] = "i_tneg";
    opcode_names[i_tvc] = "i_tvc";
    opcode_names[i_tvs] = "i_tvs";
    opcode_names[i_sub] = "i_sub";
    opcode_names[i_subcc] = "i_subcc";
    opcode_names[i_subc] = "i_subc";
    opcode_names[i_subccc] = "i_subccc";
    opcode_names[i_or] = "i_or";
    opcode_names[i_orcc] = "i_orcc";
    opcode_names[i_orn] = "i_orn";
    opcode_names[i_orncc] = "i_orncc";
    opcode_names[i_xor] = "i_xor";
    opcode_names[i_xorcc] = "i_xorcc";
    opcode_names[i_xnor] = "i_xnor";
    opcode_names[i_xnorcc] = "i_xnorcc";
    opcode_names[i_mulx] = "i_mulx";
    opcode_names[i_sdivx] = "i_sdivx";
    opcode_names[i_udivx] = "i_udivx";
    opcode_names[i_sll] = "i_sll";
    opcode_names[i_srl] = "i_srl";
    opcode_names[i_sra] = "i_sra";
    opcode_names[i_sllx] = "i_sllx";
    opcode_names[i_srlx] = "i_srlx";
    opcode_names[i_srax] = "i_srax";
    opcode_names[i_taddcc] = "i_taddcc";
    opcode_names[i_taddcctv] = "i_taddcctv";
    opcode_names[i_tsubcc] = "i_tsubcc";
    opcode_names[i_tsubcctv] = "i_tsubcctv";
    opcode_names[i_udiv] = "i_udiv";
    opcode_names[i_sdiv] = "i_sdiv";
    opcode_names[i_umul] = "i_umul";
    opcode_names[i_smul] = "i_smul";
    opcode_names[i_udivcc] = "i_udivcc";
    opcode_names[i_sdivcc] = "i_sdivcc";
    opcode_names[i_umulcc] = "i_umulcc";
    opcode_names[i_smulcc] = "i_smulcc";
    opcode_names[i_mulscc] = "i_mulscc";
    opcode_names[i_popc] = "i_popc";
    opcode_names[i_flush] = "i_flush";
    opcode_names[i_flushw] = "i_flushw";
    opcode_names[i_rd] = "i_rd";
    opcode_names[i_rdcc] = "i_rdcc";
    opcode_names[i_wr] = "i_wr";
    opcode_names[i_wrcc] = "i_wrcc";
    opcode_names[i_save] = "i_save";
    opcode_names[i_restore] = "i_restore";
    opcode_names[i_saved] = "i_saved";
    opcode_names[i_restored] = "i_restored";
    opcode_names[i_sethi] = "i_sethi";
    opcode_names[i_ldf] = "i_ldf";
    opcode_names[i_lddf] = "i_lddf";
    opcode_names[i_ldqf] = "i_ldqf";
    opcode_names[i_ldfa] = "i_ldfa";
    opcode_names[i_lddfa] = "i_lddfa";
    opcode_names[i_ldblk] = "i_ldblk";
    opcode_names[i_ldqfa] = "i_ldqfa";
    opcode_names[i_ldsb] = "i_ldsb";
    opcode_names[i_ldsh] = "i_ldsh";
    opcode_names[i_ldsw] = "i_ldsw";
    opcode_names[i_ldub] = "i_ldub";
    opcode_names[i_lduh] = "i_lduh";
    opcode_names[i_lduw] = "i_lduw";
    opcode_names[i_ldx] = "i_ldx";
    opcode_names[i_ldd] = "i_ldd";
    opcode_names[i_ldsba] = "i_ldsba";
    opcode_names[i_ldsha] = "i_ldsha";
    opcode_names[i_ldswa] = "i_ldswa";
    opcode_names[i_lduba] = "i_lduba";
    opcode_names[i_lduha] = "i_lduha";
    opcode_names[i_lduwa] = "i_lduwa";
    opcode_names[i_ldxa] = "i_ldxa";
    opcode_names[i_ldda] = "i_ldda";
    opcode_names[i_ldqa] = "i_ldqa";
    opcode_names[i_stf] = "i_stf";
    opcode_names[i_stdf] = "i_stdf";
    opcode_names[i_stqf] = "i_stqf";
    opcode_names[i_stb] = "i_stb";
    opcode_names[i_sth] = "i_sth";
    opcode_names[i_stw] = "i_stw";
    opcode_names[i_stx] = "i_stx";
    opcode_names[i_std] = "i_std";
    opcode_names[i_stfa] = "i_stfa";
    opcode_names[i_stdfa] = "i_stdfa";
    opcode_names[i_stblk] = "i_stblk";
    opcode_names[i_stqfa] = "i_stqfa";
    opcode_names[i_stba] = "i_stba";
    opcode_names[i_stha] = "i_stha";
    opcode_names[i_stwa] = "i_stwa";
    opcode_names[i_stxa] = "i_stxa";
    opcode_names[i_stda] = "i_stda";
    opcode_names[i_ldstub] = "i_ldstub";
    opcode_names[i_ldstuba] = "i_ldstuba";
    opcode_names[i_prefetch] = "i_prefetch";
    opcode_names[i_prefetcha] = "i_prefetcha";
    opcode_names[i_swap] = "i_swap";
    opcode_names[i_swapa] = "i_swapa";
    opcode_names[i_ldfsr] = "i_ldfsr";
    opcode_names[i_ldxfsr] = "i_ldxfsr";
    opcode_names[i_stfsr] = "i_stfsr";
    opcode_names[i_stxfsr] = "i_stxfsr";
    opcode_names[i_trap] = "i_trap";
    opcode_names[i_impdep1] = "i_impdep1";
    opcode_names[i_impdep2] = "i_impdep2";
    opcode_names[i_membar] = "i_membar";
    opcode_names[i_stbar] = "i_stbar";
    opcode_names[i_cmp] = "i_cmp";
    opcode_names[i_jmp] = "i_jmp";
    opcode_names[i_mov] = "i_mov";
    opcode_names[i_nop] = "i_nop";
    opcode_names[i_magic] = "i_magic";
    opcode_names[i_not] = "i_not";
    opcode_names[i_rdpr] = "i_rdpr";
    opcode_names[i_wrpr] = "i_wrpr";
    opcode_names[i_faligndata] = "i_faligndata";
    opcode_names[i_alignaddr] = "i_alignaddr";
    opcode_names[i_alignaddrl] = "i_alignaddrl";
    opcode_names[i_fzero] = "i_fzero";
    opcode_names[i_fzeros] = "i_fzeros";
    opcode_names[i_fsrc1] = "i_fsrc1";
    opcode_names[i_fcmple16] = "i_fcmple16";
    opcode_names[i_fcmpne16] = "i_fcmpne16";
    opcode_names[i_fcmple32] = "i_fcmple32";
    opcode_names[i_fcmpne32] = "i_fcmpne32";
    opcode_names[i_fcmpgt16] = "i_fcmpgt16";
    opcode_names[i_fcmpeq16] = "i_fcmpeq16";
    opcode_names[i_fcmpgt32] = "i_fcmpgt32";
    opcode_names[i_fcmpeq32] = "i_fcmpeq32";
    opcode_names[i_bshuffle] = "i_bshuffle";
    opcode_names[i_bmask] = "i_bmask";
    opcode_names[i_opcodes] = "i_opcodes";

    
}

bool inst_info_t::get_delay_slot()
{
    bool ret = false;
    if (type == IT_CONTROL)
    {
        if (branch_type == BRANCH_UNCOND && !annul)
        {
            ret = true;
        } else if (branch_type == BRANCH_COND || branch_type == BRANCH_PCOND)
        {
            if (!annul)
                ret = true;
        } else if (branch_type == BRANCH_INDIRECT)
            ret = true;
    }
    return ret;
}

const char *inst_info_t::get_opcode_name()
{
    ostringstream ostr;
    ostr << opcode_names[opcode];
    if (opcode == i_jmpl || branch_type == BRANCH_PCOND || branch_type == BRANCH_UNCOND) 
        ostr << "  " << annul << " " << hex << target << dec;
    string ret = ostr.str();
    return ret.c_str();
}

// sparc v9 isa decoder. For now, a member function of dynamic_instr_t. 
bool
inst_info_t::decode_instruction_info (addr_t phys_addr_pc, conf_object_t *cpu) {
	int64 cc0;
	int64 cc2;

	int64 cond;
	int64 mcond;

//	int64 fcn;

	int64 i;

	int64 imm22;

	int64 op;
	int64 op2;
	int64 op3;

	int64 opf;
	int64 opf_lo;
	int64 opf_hi;
//	int64 opf_test;

	int64 rcond;

	int64 rd;

	int64 rs1;
	int64 rs2;

	int64 fp_rd[3], fp_rs1[3], fp_rs2[3];
	
	int64 simm13 = 0;

	int64 x;

	int64 d16lo;
	int64 d16hi;
	int64 disp22;
	int64 disp19;
	int64 disp30;

	int64 asi;

	pc = SIM_read_register(cpu, pc_reg);
	ASSERT(SIM_logical_to_physical(cpu, Sim_DI_Instruction, pc) == phys_addr_pc);
	uint32 inst = SIM_read_phys_memory (cpu, phys_addr_pc, 4);

	op = maskBits32( inst, 30, 31 );
	op2 = maskBits32( inst, 22, 24 );
	op3 = maskBits32( inst, 19, 24 );
	rd = maskBits32( inst, 25, 29 );
	rs1 = maskBits32( inst, 14, 18 );
	rs2 = maskBits32( inst, 0, 4 );
	fp_rd [0] = rd; 
	fp_rd [1] = (maskBits32 (rd, 0, 0) << 5) | (maskBits32 (rd, 1, 5) << 1);
	fp_rd [2] = (maskBits32 (rd, 0, 0) << 5) | (maskBits32 (rd, 2, 5) << 2);
	fp_rs1 [0] = rs1;
	fp_rs1 [1] = (maskBits32 (rs1, 0, 0) << 5) | (maskBits32 (rs1, 1, 5) << 1);
	fp_rs1 [2] = (maskBits32 (rs1, 0, 0) << 5) | (maskBits32 (rs1, 2, 5) << 2);
	fp_rs2 [0] = rs2;
	fp_rs2 [1] = (maskBits32 (rs2, 0, 0) << 5) | (maskBits32 (rs2, 1, 5) << 1);
	fp_rs2 [2] = (maskBits32 (rs2, 0, 0) << 5) | (maskBits32 (rs2, 2, 5) << 2);
	opf = maskBits32( inst, 5, 13 );
	opf_hi = maskBits32( inst, 9, 13 );
	opf_lo = maskBits32( inst, 5, 8 );
//	opf = (opf_hi << 4) || opf_lo;
	cond = maskBits32( inst, 25, 28 );
	mcond = maskBits32( inst, 14, 17 );
	rcond = maskBits32( inst, 10, 12 );
	i = maskBits32( inst, 13, 13 );
	asi = maskBits32( inst, 12, 5 );
	disp19 = maskBits32(inst, 0, 18);
	disp22 = maskBits32(inst, 0, 21);
	disp30 = maskBits32(inst, 0, 29);
	d16hi = maskBits32(inst, 21, 20);
	d16lo = maskBits32(inst, 13, 0);
	cc2 = maskBits32(inst, 18, 18);
	cc0 = maskBits32 (inst, 12, 11);
	imm22 = maskBits32(inst, 21, 0);
	x = maskBits32(inst, 12, 12);
	simm13 = maskBits32(inst, 0, 12);

	int32 t_reg [RI_NUMBER];
	for (int32 k = 0; k < RI_NUMBER; k ++) 
		t_reg [k] = REG_NA; // temp reg

	switch (op) {
	// op = 0	
	case 0x0:
		switch (op2) {
		// op = 0, op2 = 0
		case 0x0: // _trap
			type = IT_EXECUTE;
			futype = FU_NONE;
			opcode = i_trap;
			break;
		
		// op = 0, op2 = 1
		case 0x1: // _bpcc
			type = IT_CONTROL;
			futype = FU_BRANCH;
			branch_type = BRANCH_PCOND;

			offset = 4 * (sign_ext64 (disp19, 18));
            target = pc + offset;

			// if (cond % 8) 
				t_reg [RS_CC] = CC;

			switch (cond) {
			case 0x0: // bpn
				branch_type = BRANCH_UNCOND;
				taken = false;
				opcode = i_bpn;
				break;
			case 0x1: // bpe
				opcode = i_bpe;
				break;
			case 0x2: // bple
				opcode = i_bple;
				break;
			case 0x3: 
				opcode = i_bpl;
				break;
			case 0x4: // bpleu
				opcode = i_bpleu;
				break;
			case 0x5: // bpcs
				opcode = i_bpcs;
				break;
			case 0x6: // bpneg	
				opcode = i_bpneg;
				break;
			case 0x7: // bpvs
				opcode = i_bpvs;
				break;
			case 0x8: // bpa	
				branch_type = BRANCH_UNCOND;
				taken = true;
				opcode = i_bpa;
				break;
			case 0x9: // bpne
				opcode = i_bpne;
				break;
			case 0xA: // bpg
				opcode = i_bpg;
				break;
			case 0xB: // bpge
				opcode = i_bpge;
				break;
			case 0xC: // bpgu
				opcode = i_bpgu;
				break;
			case 0xD: // bpcc
				opcode = i_bpcc;
				break;
			case 0xE: // bppos		
				opcode = i_bppos;
				break;
			case 0xF: // bpvc
				opcode = i_bpvc;
				break;
			default: 
				WARNING;
				break;
			} // switch cond
			break;
		
		// op = 0, op2 = 2
		case 0x2:	 // _bicc
			type = IT_CONTROL;
			futype = FU_BRANCH;
			branch_type = BRANCH_COND;

			offset = 4 * (sign_ext64 (disp22, 21));

			// if (cond % 8) 
				t_reg [RS_CC] = CC;

			switch (cond) {
			case 0x0: // bn
				branch_type = BRANCH_UNCOND;
				taken = false;
				opcode = i_bn;
				break;
			case 0x1: // be
				opcode = i_be;
				break;
			case 0x2: // ble
				opcode = i_ble;
				break;
			case 0x3: // bl
				opcode = i_bl;
				break;
			case 0x4: // bleu
				opcode = i_bleu;
				break;
			case 0x5: // bcs
				opcode = i_bcs;
				break;
			case 0x6: // bneg
				opcode = i_bneg;
				break;
			case 0x7: // bvs
				opcode = i_bvs;
				break;
			case 0x8: // ba
				branch_type = BRANCH_UNCOND;
				taken = true;
				opcode = i_ba;
				break;
			case 0x9: // bne
				opcode = i_bne;
				break;
			case 0xA: // bg
				opcode = i_bg;
				break;
			case 0xB: // bge
				opcode = i_bge;
				break;
			case 0xC: // bgu
				opcode = i_bgu;
				break;
			case 0xD: // bcc
				opcode = i_bcc;
				break;
			case 0xE: // bpos
				opcode = i_bpos;
				break;
			case 0xF: // bvc
				opcode = i_bvc;
				break;
			default: 
				WARNING;
			} // switch cond
			break;

		// op = 0, op2 = 3	
		case 0x3: // _bpr
			type = IT_CONTROL;
			futype = FU_BRANCH;
			branch_type = BRANCH_PCOND;

			t_reg [RS1] = rs1;

			offset = 4 * (sign_ext64 (d16hi << 14 | d16lo, 15));

			switch (rcond) {
			case 0x0: // reserved
				break;
			case 0x1: // brz
				opcode = i_brz;
				break;
			case 0x2: // brlez
				opcode = i_brlez;
				break;
			case 0x3: // brlz
				opcode = i_brlz;
				break;
			case 0x4: // reserved
				break;
			case 0x5: // brnz
				opcode = i_brnz;
				break;
			case 0x6: // brgz
				opcode = i_brgz;
				break;
			case 0x7: // brgez
				opcode = i_brgez;
				break;
			default: 
				WARNING;
			} // switch rcond
			break;

		// op = 0, op2 = 4	
		case 0x4: 	
			type = IT_EXECUTE;
			futype = FU_NONE;
			opcode = i_sethi;

			if (imm22 == 0 && rd == 0) opcode = i_nop;
			else { 
				if (rd == 0) 
					opcode = i_magic;
			   	else {
                    t_reg [RD] = rd;
                }
			}
			break;
		
		// op = 0, op2 = 5
		case 0x5: // _fbpfcc
			type = IT_CONTROL;	
			futype = FU_BRANCH;
			branch_type = BRANCH_PCOND;

			offset = 4 * (sign_ext64 (disp19, 18));

			switch (cond) {
			case 0x0: // fbpn	
				opcode = i_fbpn;
				break;
			case 0x1: // fbpne
				opcode = i_fbpne;
				break;
			case 0x2: // fbplg
				opcode = i_fbplg;
				break;
			case 0x3: // fbpul
				opcode = i_fbpul;
				break;
			case 0x4: // fbpl
				opcode = i_fbpl;
				break;
			case 0x5: // fbpug
				opcode = i_fbpug;
				break;
			case 0x6: // fbpg
				opcode = i_fbpg;
				break;
			case 0x7: // fbpu
				opcode = i_fbpu;
				break;
			case 0x8: // fbpa
				opcode = i_fbpa;
				break;
			case 0x9: // fbpe
				opcode = i_fbpe;
				break;
			case 0xA: // fbpue
				opcode = i_fbpue;
				break;
			case 0xB: // fbpge
				opcode = i_fbpge;
				break;
			case 0xC: // fbpuge
				opcode = i_fbpuge;
				break;
			case 0xD: // fbple
				opcode = i_fbple;
				break;
			case 0xE: // fbpule
				opcode = i_fbpule;
				break;
			case 0xF: // fbpo
				opcode = i_fbpo;
				break;
			default:
				WARNING;
			} // switch cond

			break;
		
		// op = 0, op2 = 6
		case 0x6: // _fbfcc	
			type = IT_CONTROL;
			branch_type = BRANCH_COND;
			futype = FU_BRANCH;

			offset = 4 * (sign_ext64 (disp22, 21));

			switch (cond) {
			case 0x0: // fbn
				branch_type = BRANCH_UNCOND;
				taken = false;
				opcode = i_fbn;
				break;
			case 0x1: // fbne
				opcode = i_fbne;
				break;
			case 0x2: // fblg
				opcode = i_fblg;
				break;
			case 0x3: // fbul
				opcode = i_fbul;
				break;
			case 0x4: // fbl
				opcode = i_fbl;
				break;
			case 0x5: // fbug
				opcode = i_fbug;
				break;
			case 0x6: // fbg
				opcode = i_fbg;
				break;
			case 0x7: // fbu
				opcode = i_fbu;
				break;
			case 0x8: // fba
				branch_type = BRANCH_UNCOND;
				taken = true;
				opcode = i_fba;
				break;
			case 0x9: // fbe
				opcode = i_fbe;
				break;
			case 0xA: // fbue
				opcode = i_fbue;
				break;
			case 0xB: // fbge
				opcode = i_fbge;
				break;
			case 0xC: // fbuge
				opcode = i_fbuge;
				break;
			case 0xD: // fble
				opcode = i_fble;
				break;
			case 0xE: // fbule
				opcode = i_fbule;
				break;
			case 0xF: // fbo
				opcode = i_fbo;
				break;
			default:
				WARNING;
			} // switch cond
			break;
		
		case 0x7: // -
			WARNING;
			break;
		
		default: 
			WARNING;
			break;
		} // switch op2
		break;
	
	// op = 1
	case 0x1: 
		type = IT_CONTROL;
		futype = FU_BRANCH;
		branch_type = BRANCH_CALL;
		opcode = i_call;
		t_reg [RD] = 15;  /* write the value of PC into r[15] (o7) */

		offset = 4 * (sign_ext64 (disp30, 29));
        //target = SIM_logical_to_physical(cpu, Sim_DI_Instruction, 
        //    SIM_read_register(cpu, SIM_get_register_number(cpu, "pc")) + offset);
        //target = phys_addr_pc + offset;
		break;	

	// op = 2
	case 0x2:	
		type = IT_EXECUTE;
		t_reg [RD]  = rd;
		t_reg [RS1] = rs1;
		t_reg [RS2] = (i == 0) ? rs2 : REG_NA;

		/* ALU_CC */
		if ((op3 >= 0x10) && (op3 <= 0x24))
			t_reg [RD_CC] = CC;

		/* addc/subc with or without modifying CC */
		if ( (op3 == 0x08) || (op3 == 0x18) || (op3 == 0x0c) || (op3 == 0x1c) )
			t_reg [RS_CC] = CC;

		switch (op3) {
		case 0x00: // add
			futype = FU_INTALU;
			opcode = i_add;
			break;
		case 0x01: // and
			futype = FU_INTALU;
			opcode = i_and;
			break;
		case 0x02: // or
			futype = FU_INTALU;
			if (rs1 == 0) {
				opcode = i_mov;
				t_reg [RS1] = REG_NA;
			}
			else 
				opcode = i_or;	
			break;
		case 0x03: // xor
			futype = FU_INTALU;
			opcode = i_xor;
			break;
		case 0x04: // sub
			futype = FU_INTALU;
			opcode = i_sub;
			break;
		case 0x05: // andn
			futype = FU_INTALU;
			opcode = i_andn;
			break;
		case 0x06: // orn
			futype = FU_INTALU;
			opcode = i_orn;
			break;
		case 0x07: // xnor
			futype = FU_INTALU;

			if (i == 0 && rs2 == 0) {
				opcode = i_not;
				t_reg [RS2] = REG_NA;
			}
			else 
				opcode = i_xnor;
			break;
		case 0x08: // addc
			futype = FU_INTALU;
			opcode = i_addc;
			break;
		case 0x09: // mulx 	
			futype = FU_INTMULT;
			opcode = i_mulx;
			break;
		case 0x0A: // umul
			futype = FU_INTMULT;
			opcode = i_umul;
			break;
		case 0x0B: // smul
			futype = FU_INTMULT;
			opcode = i_smul;
			break;
		case 0x0C: // subc
			futype = FU_INTALU;
			opcode = i_subc;
			break;
		case 0x0D: // udivx
			futype = FU_INTDIV;
			opcode = i_udivx;
			break;
		case 0x0E: // udiv
			futype = FU_INTDIV;
			opcode = i_udiv;
			break;
		case 0x0F: // sdiv
			futype = FU_INTDIV;
			opcode = i_sdiv;
			break;
		case 0x10: // addcc
			futype = FU_INTALU;
			opcode = i_addcc;
			break;
		case 0x11: // andcc
			futype = FU_INTALU;
			opcode = i_andcc;
			break;
		case 0x12: // orcc
			futype = FU_INTALU;
			opcode = i_orcc;
			break;
		case 0x13: // xorcc
			futype = FU_INTALU;
			opcode = i_xorcc;
			break;
		case 0x14: // subcc
			futype = FU_INTALU;

			if (rd == 0) {
				opcode = i_cmp;
				t_reg [RD] = REG_NA;
			}
			else 
				opcode = i_subcc;
			break;
		case 0x15: // andncc
			futype = FU_INTALU;
			opcode = i_andncc;
			break;
		case 0x16: // orncc
			futype = FU_INTALU;
			opcode = i_orncc;
			break;
		case 0x17: // xnorcc
			futype = FU_INTALU;
			opcode = i_xnorcc;
			break;
		case 0x18: // addccc
			futype = FU_INTALU;
			opcode = i_addccc;
			break;
		case 0x19: // -
			WARNING;
			break;
		case 0x1A: // umulcc
			futype = FU_INTMULT;
			opcode = i_umulcc;
			break;
		case 0x1B: // smulcc
			futype = FU_INTMULT;
			opcode = i_smulcc;
			break;
		case 0x1C: // subccc	
			futype = FU_INTALU;
			opcode = i_subccc;
			break;
		case 0x1D: // -
			WARNING;
			break;
		case 0x1E: // udivcc
			futype = FU_INTDIV;
			opcode = i_udivcc;
			break;
		case 0x1F: // sdivcc
			futype = FU_INTDIV;
			opcode = i_sdivcc;
			break;
		case 0x20: // taddcc
			futype = FU_INTALU;
			opcode = i_taddcc;
			break;
		case 0x21: // tsubcc
			futype = FU_INTALU;
			opcode = i_tsubcc;
			break;
		case 0x22: // taddcctv
			futype = FU_INTALU;
			opcode = i_taddcctv;
			break;
		case 0x23: // tsubcctv
			futype = FU_INTALU;
			opcode = i_tsubcctv;
			break;
		case 0x24: // mulscc
			futype = FU_INTMULT;
			opcode = i_mulscc;
			break;
		case 0x25: // sll
			futype = FU_INTALU;

			if (x) 
				opcode = i_sllx;
			else
				opcode = i_sll;
			break;
		case 0x26: // srl
			futype = FU_INTALU;

			if (x) 
				opcode = i_srlx;
			else 
				opcode = i_srl;
			break;
		case 0x27: // sra
			futype = FU_INTALU;

			if (x) 
				opcode = i_srax;
			else 
				opcode = i_sra;
			break;	
		case 0x28: 
			futype = FU_NONE;
			t_reg [RS1] = t_reg [RS2] = REG_NA;

			if (rs1 == 15 && rd == 0) {
				opcode = i_stbar;
				t_reg [RD] = REG_NA;
			} else {
				opcode = i_rd;
				t_reg [RD] = rd;
			   	if (rs1 == 2) {
				   	opcode = i_rdcc;
				   	t_reg [RS_CC] = CC;
			   	}
			}
			break;
		case 0x29: // -
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;
			WARNING;
			break;
		case 0x2A: // rdpr
			futype = FU_NONE;
			opcode = i_rdpr;
			t_reg [RS1] = t_reg [RS2] = REG_NA;
			t_reg [RD]  = rd;
			break;
		case 0x2B: // flushw
			futype = FU_NONE;
			opcode = i_flushw;
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;
			break;
		case 0x2C: // _movcc
			futype = FU_NONE;
			t_reg [RS1] = REG_NA;
			t_reg [RS_RD] = rd;

			if (cc2) {
				// integer cc 
				cc0 = cc0 << 1;

				if (cc0 != 0 && cc0 != 4) WARNING;

				t_reg [RS_CC] = CC;
					
				switch (mcond) {
				case 0x0: // movn
					opcode = i_movn;
					break;
				case 0x1: // move
					opcode = i_move;
					break;
				case 0x2: // movle
					opcode = i_movle;
					break;
				case 0x3: // movl
					opcode = i_movl;
					break;
				case 0x4: // movleu
					opcode = i_movleu;
					break;
				case 0x5: // movcs
					opcode = i_movcs;
					break;
				case 0x6: // movneg
					opcode = i_movneg;
					break;
				case 0x7: // movvs
					opcode = i_movvs;
					break;
				case 0x8: // mova
					opcode = i_mova;
					break;
				case 0x9: // movne
					opcode = i_movne;
					break;
				case 0xA: // movg
					opcode = i_movg;
					break;
				case 0xB: // movge
					opcode = i_movge;
					break;
				case 0xC: // movgu
					opcode = i_movgu;
					break;
				case 0xD: // movcc
					opcode = i_movcc;
					break;
				case 0xE: // movpos
					opcode = i_movpos;
					break;
				case 0xF: // movvc
					opcode = i_movvc;
					break;
				default: 
					WARNING;
					break;
				} // switch cond
			} else {
				// floating point cc

				switch (mcond) {
				case 0x0: // movfn
					opcode = i_movfn;
					break;
				case 0x1: // movfne
					opcode = i_movfne;
					break;
				case 0x2: // movflg
					opcode = i_movflg;
					break;
				case 0x3: // movful
					opcode = i_movful;
					break;
				case 0x4: // movfl
					opcode = i_movfl;
					break;
				case 0x5: // movfug
					opcode = i_movfug;
					break;
				case 0x6: // movfg
					opcode = i_movfg;
					break;
				case 0x7: // movfu
					opcode = i_movfu;
					break;
				case 0x8: // movfa
					opcode = i_movfa;
					break;
				case 0x9: // movfe
					opcode = i_movfe;
					break;
				case 0xA: // movfue
					opcode = i_movfue;
					break;
				case 0xB: // movfge
					opcode = i_movfge;
					break;
				case 0xC: // movfuge
					opcode = i_movfuge;
					break;
				case 0xD: // movfle
					opcode = i_movfle;
					break;
				case 0xE: // movfule
					opcode = i_movfule;
					break;
				case 0xF: // movfo
					opcode = i_movfo;
					break;
				default: 
					WARNING;
					break;
				} // switch cond
			}
			break;
		case 0x2D: // sdivx
			futype = FU_INTDIV;
			opcode = i_sdivx;
			break;
		case 0x2E: // popc 
			futype = FU_INTALU;

			if (rs1 == 0) {
				opcode = i_popc;
				t_reg [RS1] = REG_NA;
			}
			else 
				WARNING;

			break;	
		case 0x2F: // movr
			futype = FU_NONE;
			t_reg [RS_RD] = rd;

			switch (rcond) {
			case 0x0: // -
				WARNING;
				break;
			case 0x1: // movrz
				opcode = i_movrz;
				break;
			case 0x2: // movrlez
				opcode = i_movrlez;
				break;
			case 0x3: // movrlz
				opcode = i_movrlz;
				break;
			case 0x4: // -
				WARNING;
				break;
			case 0x5: // movrnz
				opcode = i_movrnz;
				break;
			case 0x6: // movrgz
				opcode = i_movrgz;
				break;
			case 0x7: // movrgez
				opcode = i_movrgez;
				break;
			default:
				WARNING;
				break;
			} // switch rcond
			break;

		case 0x30: // wr
			futype = FU_NONE;
			t_reg [RD] = REG_NA;

			if (rd == 2) {
				opcode = i_wrcc;
				t_reg [RD_CC] = CC;
			}
			else 
				opcode = i_wr;
			break;	
		case 0x31: // saved restored
		 	futype = FU_NONE;
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;

			if (rd == 0) 
				opcode = i_saved;
			else 
				opcode = i_restored;
			break;
		case 0x32: // wrpr 
			futype = FU_BRANCH;
			branch_type = BRANCH_PRIV;
			opcode = i_wrpr;
			t_reg [RD] = REG_NA;
			type = IT_CONTROL;
			break;
		case 0x33: // -
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;
			WARNING;
			break;
		case 0x34: // fpop1
			futype = FU_NONE;
			t_reg [RD]  = t_reg [RS1] = t_reg [RS2] = REG_NA;
			reg_fp [RN_RS1] = reg_fp [RN_RS2] = reg_fp [RN_RD] = true;

			if (opf < 0x50) { /* with regular patterns */
			   	int32 ptr = (opf % 4) - 1;
			   	int32 rsz = 1;
			   	if (ptr == 1) rsz = 2;
			   	if (ptr == 2) rsz = 4;

				if (opf > 0x40) { /* use both source registers */
					 t_reg [RS1] = fp_rs1 [ptr];
					 reg_sz [RN_RS1] = rsz;
				} 

				reg_sz [RN_RS2] = reg_sz [RN_RD] = rsz;
			   	t_reg [RS2] = fp_rs2 [ptr]; 
				t_reg [RD]  = fp_rd [ptr];
			} else {  /* no regular patterns: default is single fp reg */
				t_reg [RS2] = rs2;
				t_reg [RD]  = rd;
			}
			
//			opf_test = mai_instr->get_instruction_field (opf_s);
//			ASSERT (opf == opf_test);
			switch (opf) {
			case 0x001: // fmovs
				opcode = i_fmovs;
				break;
			case 0x002: // fmovd
				opcode = i_fmovd;
				break;
			case 0x003: // fmovq	
				opcode = i_fmovq;
				break;
			case 0x005: // fnegs
				futype = FU_FLOATADD;
				opcode = i_fnegs;
				break;
			case 0x006: // fnegd
				futype = FU_FLOATADD;
				opcode = i_fnegd;
				break;
			case 0x007: // fnegq 
				futype = FU_FLOATADD;
				opcode = i_fnegq;
				break;
			case 0x009: // fabss
				futype = FU_FLOATADD;
				opcode = i_fabss;
				break;
			case 0x00A: // fabsd	
				futype = FU_FLOATADD;
				opcode = i_fabsd;
				break;
			case 0x00B: // fabsq
				futype = FU_FLOATADD;
				opcode = i_fabsq;
				break;
			case 0x029: // fsqrts
				futype = FU_FLOATSQRT;
				opcode = i_fsqrts;
				break;
			case 0x02A: // fsqrtd	
				futype = FU_FLOATSQRT;
				opcode = i_fsqrtd;
				break;
			case 0x02B: // fsqrtq
				futype = FU_FLOATSQRT;
				opcode = i_fsqrtq;
				break;
			case 0x041: // fadds
				futype = FU_FLOATADD;
				opcode = i_fadds;
				break;
			case 0x042: // faddd
				futype = FU_FLOATADD;
				opcode = i_faddd;
				break;
			case 0x043: // faddq
				futype = FU_FLOATADD;
				opcode = i_faddq;
				break;
			case 0x045: // fsubs
				futype = FU_FLOATADD;
				opcode = i_fsubs;
				break;
			case 0x046: // fsubd
				futype = FU_FLOATADD;
				opcode = i_fsubd;
				break;
			case 0x047: // fsubq
				futype = FU_FLOATADD;
				opcode = i_fsubq;
				break;
			case 0x049: // fmuls
				futype = FU_FLOATMULT;
				opcode = i_fmuls;
				break;
			case 0x04A: // fmuld
				futype = FU_FLOATMULT;
				opcode = i_fmuld;
				break;
			case 0x04B: // fmulq
				futype = FU_FLOATMULT;
				opcode = i_fmulq;
				break;
			case 0x04D: // fdivs
				futype = FU_FLOATDIV;
				opcode = i_fdivs;
				break;
			case 0x04E: // fdivd
				futype = FU_FLOATDIV;
				opcode = i_fdivd;
				break;
			case 0x04F: // fdivq
				futype = FU_FLOATDIV;
				opcode = i_fdivq;
				break;
			case 0x069: // fsmuld
				futype = FU_FLOATMULT;
				opcode = i_fsmuld;
				t_reg [RD] = fp_rd [1];
				reg_sz [RN_RD] = 2;
				break;
			case 0x06E: // fdmulq
				futype = FU_FLOATMULT;
				opcode = i_fdmulq;
				t_reg [RS1] = fp_rs1 [1]; t_reg [RS2] = fp_rs2 [1]; t_reg [RD] = fp_rd [2];
				reg_sz [RN_RS1] = reg_sz [RN_RS2] = 2; reg_sz [RN_RD] = 4;
				break;
			case 0x081: // fstox
				futype = FU_FLOATCVT;
				opcode = i_fstox;
				break;
			case 0x082: // fdtox
				futype = FU_FLOATCVT;
				opcode = i_fdtox;
				t_reg [RS2] = fp_rs2 [1];
				reg_sz [RN_RS2] = 2; 
				break;
			case 0x083: // fqtox 
				futype = FU_FLOATCVT;
				opcode = i_fqtox;
				t_reg [RS2] = fp_rs2 [2];
				reg_sz [RN_RS2] = 4; 
				break;
			case 0x084: // fxtos	
				futype = FU_FLOATCVT;
				opcode = i_fxtos;
				break;
			case 0x088: // fxtod
				futype = FU_FLOATCVT;
				opcode = i_fxtod;
				t_reg [RD] = fp_rd [1];
				reg_sz [RN_RD] = 2; 
				break;
			case 0x08C: // fxtoq
				futype = FU_FLOATCVT;
				opcode = i_fxtoq;
				t_reg [RD] = fp_rd [2];
				reg_sz [RN_RD] = 4; 
				break;
			case 0x0C4: // fitos
				futype = FU_FLOATCVT;
				opcode = i_fitos;
				break;
			case 0x0C6: // fdtos
				futype = FU_FLOATCVT;
				opcode = i_fdtos;
				t_reg [RS2] = fp_rs2 [1]; 
				reg_sz [RN_RS2] = 2;
				break;
			case 0x0C7: // fqtos
				futype = FU_FLOATCVT;
				opcode = i_fqtos;
				t_reg [RS2] = fp_rs2 [2];
				reg_sz [RN_RS2] = 4;
				break;
			case 0x0C8: // fitod
				futype = FU_FLOATCVT;
				opcode = i_fitod;
				t_reg [RD] = fp_rd [1];
				reg_sz [RN_RD] = 2;
				break;
			case 0x0C9: // fstod
				futype = FU_FLOATCVT;
				opcode = i_fstod;
				t_reg [RD] = fp_rd [1];
				reg_sz [RN_RD] = 2;
				break;
			case 0x0CB: // fqtod
				futype = FU_FLOATCVT;
				opcode = i_fqtod;
				t_reg [RS2] = fp_rs2 [2]; t_reg [RD] = fp_rd [1];
				reg_sz [RN_RS2] = 4; reg_sz [RN_RD] = 2;
				break;
			case 0x0CC: // fitoq
				futype = FU_FLOATCVT;
				opcode = i_fitoq;
				t_reg [RD] = fp_rd [2];
				reg_sz [RN_RD] = 4;
				break;
			case 0x0CD: // fstoq
				futype = FU_FLOATCVT;
				opcode = i_fstoq;
				t_reg [RD] = fp_rd [2];
				reg_sz [RN_RD] = 4;
				break;
			case 0x0CE: // fdtoq
				futype = FU_FLOATCVT;
				opcode = i_fdtoq;
				t_reg [RS2] = fp_rs2 [1]; t_reg [RD] = fp_rd [2];
				reg_sz [RN_RS2] = 2; reg_sz [RN_RD] = 4;
				break;
			case 0x0D1: // fstoi 
				opcode = i_fstoi;
				break;
			case 0x0D2: // fdtoi
				opcode = i_fdtoi;
				t_reg [RS2] = fp_rs2 [1];
				reg_sz [RN_RS2] = 2;
				break;
			case 0x0D3: // fqtoi;
				opcode = i_fqtoi;
				t_reg [RS2] = fp_rs2 [2];
				reg_sz [RN_RS2] = 4;
				break;
			default:
				WARNING;
				break;
			} // switch opf	
			break;

		case 0x35: // fpop2
			futype = FU_NONE;
			t_reg [RD]  = t_reg [RS1] = t_reg [RS2] = REG_NA;
			reg_fp [RN_RS1] = reg_fp [RN_RS2] = reg_fp [RN_RS_RD] = reg_fp [RN_RD] = true;

			{
			   	int32 ptr = (opf % 4) - 1;
			   	int32 rsz = 1;
			   	if (ptr == 1) rsz = 2;
			   	if (ptr == 2) rsz = 4;

				t_reg [RS2] = fp_rs2 [ptr]; t_reg [RD]  = fp_rd [ptr];
			   	reg_sz [RN_RS2] = reg_sz [RN_RD] = rsz;

				/* fmov dependent on integer CC */
				if ( (opf_hi == 0x10) || (opf_hi == 0x18) ) 
					t_reg [RS_CC] = CC;
				
				if (opf_hi == 0x5) {
				   	/* FCMP */
				   	t_reg [RD] = REG_NA;
				   	t_reg [RS1] = fp_rs1 [ptr];
				   	reg_sz [RN_RS1] = rsz;
			   	} else if (opf_lo > 0x4) {
				   	/* FMOVR */
				   	t_reg [RS1] = rs1;
				   	reg_sz [RN_RS1] = 1;
				   	reg_fp [RN_RS1] = false;
			   	}
			}	
			
//			opf_test = mai_instr->get_instruction_field (opf_s);
//			ASSERT (opf_test == opf);

			switch (opf) {
			case 0x001: case 0x041: case 0x081: case 0x0C1:
				// fcc0, fcc1, fcc2, fcc3 and fmovfs operations

				switch (mcond) {
				case 0x0: // fmovfsn
					opcode = i_fmovfsn;
					break;
				case 0x1: // fmovfsne
					opcode = i_fmovfsne;
					break;
				case 0x2: // fmovfslg
					opcode = i_fmovfslg;
					break;
				case 0x3: // fmovfsul
					opcode = i_fmovfsul;
					break;
				case 0x4: // fmovfsl
					opcode = i_fmovfsl;
					break;
				case 0x5: // fmovfsug
					opcode = i_fmovfsug;
					break;
				case 0x6: // fmovfsg
					opcode = i_fmovfsg;
					break;
				case 0x7: // fmovfsu
					opcode = i_fmovfsu;
					break;
				case 0x8: // fmovfsa
					opcode = i_fmovfsa;
					break;
				case 0x9: // fmovfse
					opcode = i_fmovfse;
					break;
				case 0xA: // fmovfsue
					opcode = i_fmovfsue;
					break;
				case 0xB: // fmovfsge
					opcode = i_fmovfsge;
					break;
				case 0xC: // fmovfsuge
					opcode = i_fmovfsuge;
					break;
				case 0xD: // fmovfsle
					opcode = i_fmovfsle;
					break;
				case 0xE: // fmovfsule
					opcode = i_fmovfsule;
					break;
				case 0xF: // fmovfso
					opcode = i_fmovfso;
					break;
				} // switch cond
				break;

			case 0x002: case 0x042: case 0x082: case 0x0C2:
				// fcc0, fcc1, fcc2, fcc3 and fmovfd operations
				
				switch (mcond) {
				case 0x0: // fmovfdn
					opcode = i_fmovfdn;
					break;
				case 0x1: // fmovfdne
					opcode = i_fmovfdne;
					break;
				case 0x2: // fmovfdlg
					opcode = i_fmovfdlg;
					break;
				case 0x3: // fmovfdul
					opcode = i_fmovfdul;
					break;
				case 0x4: // fmovfdl
					opcode = i_fmovfdl;
					break;
				case 0x5: // fmovfdug
					opcode = i_fmovfdug;
					break;
				case 0x6: // fmovfdg
					opcode = i_fmovfdg;
					break;
				case 0x7: // fmovfdu
					opcode = i_fmovfdu;
					break;
				case 0x8: // fmovfda
					opcode = i_fmovfda;
					break;
				case 0x9: // fmovfde
					opcode = i_fmovfde;
					break;
				case 0xA: // fmovfdue
					opcode = i_fmovfdue;
					break;
				case 0xB: // fmovfdge
					opcode = i_fmovfdge;
					break;
				case 0xC: // fmovfduge
					opcode = i_fmovfduge;
					break;
				case 0xD: // fmovfdle
					opcode = i_fmovfdle;
					break;
				case 0xE: // fmovfdule
					opcode = i_fmovfdule;
					break;
				case 0xF: // fmovfdo
					opcode = i_fmovfdo;
					break;
				} // switch cond
				break;

			case 0x003: case 0x043: case 0x083: case 0x0C3:
				// fcc0, fcc1, fcc2, fcc3 and fmovfq operations

				switch (mcond) {
				case 0x0: // fmovfqn
					opcode = i_fmovfqn;
					break;
				case 0x1: // fmovfqne
					opcode = i_fmovfqne;
					break;
				case 0x2: // fmovfqlg
					opcode = i_fmovfqlg;
					break;
				case 0x3: // fmovfqul
					opcode = i_fmovfqul;
					break;
				case 0x4: // fmovfql
					opcode = i_fmovfql;
					break;
				case 0x5: // fmovfqug
					opcode = i_fmovfqug;
					break;
				case 0x6: // fmovfqg
					opcode = i_fmovfqg;
					break;
				case 0x7: // fmovfqu
					opcode = i_fmovfqu;
					break;
				case 0x8: // fmovfqa
					opcode = i_fmovfqa;
					break;
				case 0x9: // fmovfqe
					opcode = i_fmovfqe;
					break;
				case 0xA: // fmovfque
					opcode = i_fmovfque;
					break;
				case 0xB: // fmovfqge
					opcode = i_fmovfqge;
					break;
				case 0xC: // fmovfquge
					opcode = i_fmovfquge;
					break;
				case 0xD: // fmovfqle
					opcode = i_fmovfqle;
					break;
				case 0xE: // fmovfqule
					opcode = i_fmovfqule;
					break;
				case 0xF: // fmovfqo
					opcode = i_fmovfqo;
					break;
				} // switch cond
				break;

			case 0x101: case 0x181:
				// icc, xcc and fmovs operations

				switch (mcond) {
				case 0x0:  // fmovsn
					opcode = i_fmovsn;
					break;
				case 0x1: // fmovse
					opcode = i_fmovse;
					break;
				case 0x2: // fmovsle
					opcode = i_fmovsle;
					break;
				case 0x3: // fmovsl
					opcode = i_fmovsl;
					break;
				case 0x4: // fmovsleu
					opcode = i_fmovsleu;
					break;
				case 0x5: // fmovscs
					opcode = i_fmovscs;
					break;
				case 0x6: // fmovsneg
					opcode = i_fmovsneg;
					break;
				case 0x7: // fmovsvs
					opcode = i_fmovsvs;
					break;
				case 0x8: // fmovsa
					opcode = i_fmovsa;
					break;
				case 0x9: // fmovsne
					opcode = i_fmovsne;
					break;
				case 0xA: // fmovsg
					opcode = i_fmovsg;
					break;
				case 0xB: // fmovsge
					opcode = i_fmovsge;
					break;
				case 0xC: // fmovsgu
					opcode = i_fmovsgu;
					break;
				case 0xD: // fmovscc
					opcode = i_fmovscc;
					break;
				case 0xE: // fmovspos
					opcode = i_fmovspos;
					break;
				case 0xF: // fmovsvc
					opcode = i_fmovsvc;
					break;
				} // switch cond
				break;

			case 0x102: case 0x182: 
				// icc, xcc and fmovd operations
				
				switch (mcond) {
				case 0x0: // fmovdn
					opcode = i_fmovdn;
					break;
				case 0x1: // fmovde
					opcode = i_fmovde;
					break;
				case 0x2: // fmovdle
					opcode = i_fmovdle;
					break;
				case 0x3: // fmovdl
					opcode = i_fmovdl;
					break;
				case 0x4: // fmovdleu
					opcode = i_fmovdleu;
					break;
				case 0x5: // fmovdcs
					opcode = i_fmovdcs;
					break;
				case 0x6: // fmovdneg
					opcode = i_fmovdneg;
					break;
				case 0x7: // fmovdvs
					opcode = i_fmovdvs;
					break;
				case 0x8: // fmovda
					opcode = i_fmovda;
					break;
				case 0x9: // fmovdne
					opcode = i_fmovdne;
					break;
				case 0xA: // fmovdg
					opcode = i_fmovdg;
					break;
				case 0xB: // fmovdge
					opcode = i_fmovdge;
					break;
				case 0xC: // fmovdgu
					opcode = i_fmovdgu;
					break;
				case 0xD: // fmovdcc
					opcode = i_fmovdcc;
					break;
				case 0xE: // fmovdpos
					opcode = i_fmovdpos;
					break;
				case 0xF: // fmovdvc
					opcode = i_fmovdvc;
					break;
				} // switch cond
				break;
			
			case 0x103: case 0x183:

				switch (mcond) {
				// icc, xcc and fmovq operations
				case 0x0: // fmovqn
					opcode = i_fmovqn;
					break;
				case 0x1: // fmovqe
					opcode = i_fmovqe;
					break;
				case 0x2: // fmovqle
					opcode = i_fmovqle;
					break;
				case 0x3: // fmovql
					opcode = i_fmovql;
					break;
				case 0x4: // fmovqleu
					opcode = i_fmovqleu;
					break;
				case 0x5: // fmovqcs
					opcode = i_fmovqcs;
					break;
				case 0x6: // fmovqneg
					opcode = i_fmovqneg;
					break;
				case 0x7: // fmovqvs
					opcode = i_fmovqvs;
					break;
				case 0x8: // fmovqa
					opcode = i_fmovqa;
					break;
				case 0x9: // fmovqne
					opcode = i_fmovqne;
					break;
				case 0xA: // fmovqg
					opcode = i_fmovqg;
					break;
				case 0xB: // fmovqge
					opcode = i_fmovqge;
					break;
				case 0xC: // fmovqgu
					opcode = i_fmovqgu;
					break;
				case 0xD: // fmovqcc
					opcode = i_fmovqcc;
					break;
				case 0xE: // fmovqpos
					opcode = i_fmovqpos;
					break;
				case 0xF: // fmovqvc
					opcode = i_fmovqvc;
					break;
				} // switch cond
				break;

			case 0x051: // fcmps
				futype = FU_FLOATCMP;
				opcode = i_fcmps;
				break;
			case 0x052: // fcmpd
				futype = FU_FLOATCMP;
				opcode = i_fcmpd;
				break;
			case 0x053: // fcmpq
				futype = FU_FLOATCMP;
				opcode = i_fcmpq;
				break;
			case 0x055: // fcmpes
				futype = FU_FLOATCMP;
				opcode = i_fcmpes;
				break;
			case 0x056: // fcmped
				futype = FU_FLOATCMP;
				opcode = i_fcmped;
				break;
			case 0x057: // fcmpeq
				futype = FU_FLOATCMP;
				opcode = i_fcmpeq;
				break;

			case 0x025: // fmovrsz
				opcode = i_fmovrsz;
				break;
			case 0x026: // fmovrdz
				opcode = i_fmovrdz;
				break;
			case 0x027: // fmovrqz
				opcode = i_fmovrqz;
				break;
			case 0x045: // fmovrslez
				opcode = i_fmovrslez;
				break;
			case 0x046: // fmovrdlez
				opcode = i_fmovrdlez;
				break;
			case 0x047: // fmovrqlez
				opcode = i_fmovrqlez;
				break;
			case 0x0A5: // fmovrsnz
				opcode = i_fmovrsnz;
				break;
			case 0x0A6: // fmovrdnz
				opcode = i_fmovrdnz;
				break;
			case 0x0A7: // fmovrqlz
				opcode = i_fmovrqlz;
				break;
			case 0x0C5: // fmovrsgz
				opcode = i_fmovrsgz;
				break;
			case 0x0C6: // fmovrdgz
				opcode = i_fmovrdgz;
				break;
			case 0x0C7: // fmovrqgz
				opcode = i_fmovrqgz;
				break;
			case 0x0E5: // fmovrsgez
				opcode = i_fmovrsgez;
				break;
			case 0x0E6: // fmovrdgez
				opcode = i_fmovrdgez;
				break;
			case 0x0E7: // fmovrqgez
				opcode = i_fmovrqgez;
				break;
			} // switch opf
			break;

		case 0x36: // impdep1
			futype = FU_NONE;
			opcode = i_impdep1;
			break;

		case 0x37: // impdep2
			futype = FU_NONE;
			opcode = i_impdep2;
			break;

		case 0x38:	// jmpl
			type = IT_CONTROL;
			futype = FU_NONE;
			branch_type = BRANCH_INDIRECT;
			opcode = i_jmpl;
            rd_reg = rd;

			if ( (rd == 0) && 
			     (i == 1 && (rs1 == 15 || rs1 == 31) &&	8 == simm13) ) {
				branch_type = BRANCH_RETURN;
			} else if (rd == 15) {
				/* register indirect call */
				branch_type = BRANCH_CALL;
                if (i == 1) {
                    target = SIM_read_register(cpu, rs1) + simm13;
                } else if (i == 0) {
                    target = SIM_read_register(cpu, rs1) + SIM_read_register(cpu, rs2);
                } else
                    FAIL;
                    
			}
            
            if (i == 1) {
                target = SIM_read_register(cpu, rs1) + simm13;
            } else if (i == 0) {
                target = SIM_read_register(cpu, rs1) + SIM_read_register(cpu, rs2);
            } else
            FAIL;
            
			break;
		case 0x39: // return	
			type = IT_CONTROL;
			futype = FU_BRANCH;
			branch_type = BRANCH_RETURN;
			opcode = i_retrn;
			break;
		
		case 0x3A: // tcc
			type = IT_CONTROL;
			futype = FU_BRANCH;
			branch_type = BRANCH_TRAP;
			t_reg [RD] = REG_NA;

			// if (cond % 8) 
				t_reg [RS_CC] = CC;

			switch (cond) {
			case 0x0: // tn
				opcode = i_tn;
				break;
			case 0x1: // te
				opcode = i_te;
				break;
			case 0x2: // tle
				opcode = i_tle;
				break;
			case 0x3: // tl
				opcode = i_tl;
				break;
			case 0x4: // tleu
				opcode = i_tleu;
				break;
			case 0x5: // tcs
				opcode = i_tcs;
				break;
			case 0x6: // tneg
				opcode = i_tneg;
				break;
			case 0x7: // tvs
				opcode = i_tvs;
				break;
			case 0x8: // ta
				opcode = i_ta;
				break;
			case 0x9: // tne
				opcode = i_tne;
				break;
			case 0xA: // tg
				opcode = i_tg;
				break;
			case 0xB: // tge
				opcode = i_tge;
				break;
			case 0xC: // tgu
				opcode = i_tgu;
				break;
			case 0xD: // tcc
				opcode = i_tcc;
				break;
			case 0xE: // tpos
				opcode = i_tpos;
				break;
			case 0xF: // tvc
				opcode = i_tvc;
				break;
			default:
				WARNING;
				break;
			} // switch cond
			break;

		case 0x3B: // flush
			futype = FU_NONE;
			opcode = i_flush;
			t_reg [RD] = REG_NA;
			break;

		case 0x3C: // save	
			futype = FU_BRANCH;
			type = IT_CONTROL;
			branch_type = BRANCH_CWP;
			opcode = i_save;
			break;
		
		case 0x3D: // restore
			futype = FU_BRANCH;
			type = IT_CONTROL;
			branch_type = BRANCH_CWP;
			opcode = i_restore;
			break;

		case 0x3E: // done retry	
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;

			switch (rd) {
			case 0x0: 	
				opcode = i_done;
				branch_type = BRANCH_TRAP_RETURN;
				futype = FU_BRANCH;
				type = IT_CONTROL;
				break;
			
			case 0x1: 
				opcode = i_retry;
				type = IT_CONTROL;
				branch_type = BRANCH_TRAP_RETURN;
				futype = FU_BRANCH;
				break;
			} // switch fcn
			break;

		default: 	
			t_reg [RS1] = t_reg [RS2] = t_reg [RD] = REG_NA;
			WARNING;
			break;
		} // switch op3, op = 2
		break;
	
	case 0x3: 
		t_reg [RS1] = rs1; t_reg [RS2] = (i == 0) ? rs2 : REG_NA; t_reg [RD] = rd;

		switch (op3) {
		case 0x00: // lduw
			type = IT_LOAD;
			opcode = i_lduw;
			futype = FU_MEMRDPORT;
			break;
		case 0x01: // ldub
			type = IT_LOAD;
			opcode = i_ldub;
			futype = FU_MEMRDPORT;
			break;
		case 0x02: // lduh
			type = IT_LOAD;
			opcode = i_lduh;
			futype = FU_MEMRDPORT;
			break;
		case 0x03: // ldd
			type = IT_LOAD;
			opcode = i_ldd;
			futype = FU_MEMRDPORT;
			reg_sz [RN_RD] = 2;
			break;
		case 0x04: // stw
			type = IT_STORE;
			opcode = i_stw;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			t_reg [RD] = REG_NA;
			break;
		case 0x05: // stb
			type = IT_STORE;
			opcode = i_stb;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			t_reg [RD] = REG_NA;
			break;
		case 0x06: // sth
			type = IT_STORE;
			opcode = i_sth;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			t_reg [RD] = REG_NA;
			break;
		case 0x07: // std
			type = IT_STORE;
			opcode = i_std;
			ainfo = AI_ATOMIC;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 2;
			t_reg [RS_RD] = rd;
			t_reg [RD] = REG_NA;
			break;
		case 0x08: // ldsw
			type = IT_LOAD;
			opcode = i_ldsw;
			futype = FU_MEMRDPORT;
			break;
		case 0x09: // ldsb
			type = IT_LOAD;
			opcode = i_ldsb;
			futype = FU_MEMRDPORT;
			break;
		case 0x0A: // ldsh
			type = IT_LOAD;
			opcode = i_ldsh;
			futype = FU_MEMRDPORT;
			break;
		case 0x0B: // ldx
			type = IT_LOAD;
			opcode = i_ldx;
			futype = FU_MEMRDPORT;
			break;
		case 0x0C: // -
			WARNING;
			break;
		case 0x0D: // ldstub
			type = IT_STORE;
			ainfo = AI_ATOMIC;
			opcode = i_ldstub;
			futype = FU_MEMWRPORT;
			break;
		case 0x0E: // stx
			type = IT_STORE;
			opcode = i_stx;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			t_reg [RD] = REG_NA;
			break;
		case 0x0F: // swap
			type = IT_STORE;
			ainfo = AI_ATOMIC;
			opcode = i_swap;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			break;
		case 0x10: // lduwa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_lduwa;
			futype = FU_MEMRDPORT;
			break;
		case 0x11: // lduba
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_lduba;
			futype = FU_MEMRDPORT;
			break;
		case 0x12: // lduha
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_lduha;
			futype = FU_MEMRDPORT;
			break;
		case 0x13: // ldda
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldda;
			futype = FU_MEMRDPORT;
			reg_sz [RN_RD] = 2;
				
			if (i == 0) {
				imm_asi = asi;
				if (asi == 0x24 || asi == 0x2c) {
					ainfo |= AI_ATOMIC;
					reg_sz [RN_RD] = 2; /* FIXME: should be 4 */
				}
			}

			break;
		case 0x14: // stwa
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stwa;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd; 
			t_reg [RD] = REG_NA;
			break;
		case 0x15: // stba
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stba;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd; 
			t_reg [RD] = REG_NA;
			break;
		case 0x16: // stha
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stha;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd; 
			t_reg [RD] = REG_NA;
			break;
		case 0x17: // stda
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stda;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 2;
			t_reg [RS_RD] = rd; 
			t_reg [RD] = REG_NA;
			break;
		case 0x18: // ldswa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldswa;
			futype = FU_MEMRDPORT;
			break;
		case 0x19: // ldsba
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldsba;
			futype = FU_MEMRDPORT;
			break;
		case 0x1A: // ldsha
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldsha;
			futype = FU_MEMRDPORT;
			break;
		case 0x1B: // ldxa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldxa;
			futype = FU_MEMRDPORT;
			break;
		case 0x1C: // -
			WARNING;
			break;
		case 0x1D: // ldstuba
			type = IT_STORE;
			opcode = i_ldstuba;
			ainfo = AI_ATOMIC | AI_ALT_SPACE;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd;
			break;
		case 0x1E: // stxa
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stxa;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd; 
			t_reg [RD] = REG_NA;
			break;
		case 0x1F: // swapa	
			type = IT_STORE;
			ainfo = AI_ATOMIC | AI_ALT_SPACE;
			opcode = i_swapa;
			futype = FU_MEMWRPORT;
			t_reg [RS_RD] = rd; 
			break;
		case 0x20: // ldf
			type = IT_LOAD;
			opcode = i_ldf;
			futype = FU_MEMRDPORT;
			reg_fp [RN_RD] = true;
			break;
		case 0x21: // ldfsr
			type = IT_LOAD;

			if (rd == 0) {
				opcode = i_ldfsr;
			} else if (rd == 1) {
				opcode = i_ldxfsr;
			}
			futype = FU_MEMRDPORT;
			t_reg [RD] = REG_NA;
			break;
		case 0x22: // ldqf	
			type = IT_LOAD;
			opcode = i_ldqf;
			futype = FU_MEMRDPORT;
			t_reg [RD] = fp_rd [2];
			reg_sz [RN_RD] = 4;
			reg_fp [RN_RD] = true;
			break;
		case 0x23: // lddf
			type = IT_LOAD;
			opcode = i_lddf;
			futype = FU_MEMRDPORT;
			t_reg [RD] = fp_rd [1];
			reg_sz [RN_RD] = 2;
			reg_fp [RN_RD] = true;
			break;
		case 0x24: // stf
			type = IT_STORE;
			opcode = i_stf;
			futype = FU_MEMWRPORT;
			reg_fp [RN_RS_RD] = true;
		   	t_reg [RS_RD] = rd;
		   	t_reg [RD] = REG_NA;
			break;
		case 0x25: // stfsr
			type = IT_STORE;
			if (rd == 0) {
				opcode = i_stfsr;
			} else if (rd == 1) {
				opcode = i_stxfsr;
			}
			futype = FU_MEMWRPORT;
			t_reg [RD] = REG_NA;
			break;
		case 0x26: // stqf
			type = IT_STORE;
			opcode = i_stqf;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 4;
			reg_fp [RN_RS_RD] = true;
			t_reg [RS_RD] = fp_rd [2];
			t_reg [RD] = REG_NA;
			break;
		case 0x27: // stdf
			type = IT_STORE;
			opcode = i_stdf;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 2;
			reg_fp [RN_RS_RD] = true;
			t_reg [RS_RD] = fp_rd [1];
			t_reg [RD] = REG_NA;
			break;
		case 0x2D: // prefetch
			type = IT_LOAD;
			ainfo = AI_PREFETCH;
			futype = FU_MEMRDPORT;
			opcode = i_prefetch;
			t_reg [RD] = REG_NA;
			break;
		case 0x30: // ldfa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldfa;
			futype = FU_MEMRDPORT;
			reg_fp [RN_RD] = true;
			break;
		case 0x32: // ldqfa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_ldqfa;
			futype = FU_MEMRDPORT;
			t_reg [RD] = fp_rd [2];
			reg_sz [RN_RD] = 4;
			reg_fp [RN_RD] = true;
		case 0x33: // lddfa
			type = IT_LOAD;
			ainfo = AI_ALT_SPACE;
			opcode = i_lddfa;
			futype = FU_MEMRDPORT;
			reg_fp [RN_RD] = true;
			t_reg [RD] = fp_rd [1];

			if (i == 0)
				imm_asi = asi;
            
			//else imm_asi = mai_instr->read_input_asi ();

			if (imm_asi == 0x70 || imm_asi == 0x71 || imm_asi == 0x78 || imm_asi == 0x79
					|| imm_asi == 0xe0 || imm_asi == 0xe1 || imm_asi == 0xf0 || imm_asi == 0xf1 
					|| imm_asi == 0xf8 || imm_asi == 0xf9) 
					reg_sz [RN_RD] = 16;
			else reg_sz [RN_RD] = 2;

			break;
		case 0x34: // stfa
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stfa;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 2;
			reg_fp [RN_RS_RD] = true;
		   	t_reg [RS_RD] = rd;
		   	t_reg [RD] = REG_NA;
			break;
		case 0x36: // stqfa
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stqfa;
			futype = FU_MEMWRPORT;
			reg_sz [RN_RS_RD] = 4;
			reg_fp [RN_RS_RD] = true;
			t_reg [RS_RD] = fp_rd [2];
			t_reg [RD] = REG_NA;
			break;
		case 0x37: // stdfa
			type = IT_STORE;
			ainfo = AI_ALT_SPACE;
			opcode = i_stdfa;
			futype = FU_MEMWRPORT;
			t_reg [RD] = REG_NA;
			t_reg [RS_RD] = fp_rd [1];
			reg_fp [RN_RS_RD] = true;

			if (i == 0)
				imm_asi = asi;
			//else imm_asi = mai_instr->read_input_asi ();

			if (imm_asi == 0x70 || imm_asi == 0x71 || imm_asi == 0x78 || imm_asi == 0x79
					|| imm_asi == 0xe0 || imm_asi == 0xe1 || imm_asi == 0xf0 || imm_asi == 0xf1 
					|| imm_asi == 0xf8 || imm_asi == 0xf9) 
					reg_sz [RN_RS_RD] = 16;
			else reg_sz [RN_RS_RD] = 2;
			break;
		case 0x3C: // casa
			type = IT_STORE;
			ainfo = AI_ATOMIC | AI_ALT_SPACE;
			futype = FU_MEMWRPORT;
			opcode = i_casa;
			t_reg [RS_RD] = rd;
			t_reg [RD]  = rd;
			break;
		case 0x3D: // prefetcha
			type = IT_LOAD;
			ainfo = AI_ATOMIC | AI_PREFETCH | AI_ALT_SPACE;
			futype = FU_MEMRDPORT; 
			opcode = i_prefetcha;
			t_reg [RD] = REG_NA;
			break;
		case 0x3E: // casxa
			type = IT_STORE;
			ainfo = AI_ATOMIC | AI_ALT_SPACE;
			futype = FU_MEMWRPORT;
			opcode = i_casxa;
			t_reg [RS_RD] = rd;
			t_reg [RD]  = rd;
			break;
		default: // -	
            cerr << "Physical PC passed " << hex << phys_addr_pc << dec << endl;
			WARNING;
			break;
		} // switch op3, op = 3
		break;
	default: 
		// WARNING;
		WARNING;
		break;
	} // switch op 

	if (futype == FU_BRANCH && (
		branch_type == BRANCH_UNCOND ||
		branch_type == BRANCH_COND || 
		branch_type == BRANCH_PCOND)) {
		annul = maskBits32 (inst, 29, 29);
	}

	/* adjust for fp, and dr/qr */
	for (int32 k = 0; k < RN_NUMBER; k ++) {
		int32 ii = regbox_index[k];
		if ( (t_reg [ii] != REG_NA) && (reg_fp [k]) )
			t_reg [ii] += 32;

		int32 reg_no = t_reg [ii];
		if ( (reg_no) && (reg_no != REG_NA) ) 
			for (int32 nr = 1; nr < reg_sz [k]; nr ++)
				t_reg [ii + nr] = t_reg [ii] + nr;
	}

	/* get number of src/dest, and pack the l_reg array */
	for (int32 k = 0; k < RD; k++) 
		if ((t_reg [k] != REG_NA) && (t_reg [k]) /* special for g0 */) 
			l_reg [num_srcs ++] = t_reg [k];

   	for (int32 k = RD; k < RI_NUMBER; k++) 
	   	if ((t_reg [k] != REG_NA) && (t_reg [k]) /* special for g0 */) {
			l_reg [RD + num_dests] = t_reg [k];
			num_dests ++;
		}
			

	// failed
	if (type == IT_TYPES || futype == FU_TYPES) return false;
	if (type == IT_CONTROL && branch_type == BRANCH_TYPES) return false;

	// successfully decoded
	return true;	

}


void inst_info_t::read_output_regs(conf_object_t *cpu, int64 *regs)
{
	for (int32 i = 0; i < num_dests; i++)
		regs[i] = SIM_read_register(cpu, l_reg[RD + i]);
}

bool inst_info_t::is_possible_bct()
{
	return (futype == FU_BRANCH && (
		branch_type == BRANCH_UNCOND ||
		branch_type == BRANCH_COND || 
		branch_type == BRANCH_PCOND || 
		branch_type == BRANCH_INDIRECT));
}

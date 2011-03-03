


#ifndef _AREA_CACTI_H_
#define _AREA_CACTI_H_


class area_t {
    
    
    public:



        double logtwo_area (double x);
        area_type inverter_area (double Widthp,double Widthn);
        area_type subarraymem_area (int C,int B,int A,int Ndbl,int Ndwl,
            double Nspd,int RWP,int ERP,int EWP,int NSER,double techscaling_factor);
        
        area_type decodemem_row (int C,int B,int A,int Ndbl,double Nspd,int Ndwl,
            int RWP,int ERP,int EWP);
        area_type predecode_area (double noof_rows,int RWP,int ERP,int EWP);
        area_type postdecode_area (int noof_rows,int RWP,int ERP,int EWP);
        area_type colmux (int Ndbl,double Nspd,int RWP,int ERP,int EWP,int NSER);
        area_type precharge (int Ndbl,double Nspd,int RWP,int ERP,int EWP,int NSER);
        area_type senseamp (int Ndbl,double Nspd,int RWP,int ERP,int EWP,int NSER);
        area_type subarraytag_area (int baddr,int C,int B,int A,int Ntdbl,int Ntdwl,
            int Ntspd,double NSubbanks,int RWP,int ERP,int EWP,int NSER,
            double techscaling_factor);
        area_type decodetag_row (int baddr,int C,int B,int A,int Ntdbl,int Ntspd,
            int Ntdwl,double NSubbanks,int RWP,int ERP,int EWP);
        
        area_type comparatorbit (int RWP,int ERP,int EWP);
        area_type muxdriverdecode (int B,int b0,int RWP,int ERP,int EWP);
        area_type muxdrvsig (int A,int B,int b0);
        area_type datasubarray (int C,int B,int A,int Ndbl,int Ndwl,double Nspd,
            int RWP,int ERP,int EWP,int NSER,  double techscaling_factor);
        
        area_type datasubblock (int C,int B,int A,int Ndbl,int Ndwl,double Nspd,
            int SB,int b0,int RWP,int ERP,int EWP,int NSER, double techscaling_factor);
        
        area_type dataarray (int C,int B,int A,int Ndbl,int Ndwl,double Nspd,
            int b0,int RWP,int ERP,int EWP,int NSER,  double techscaling_factor);
        area_type tagsubarray (int baddr,int C,int B,int A,int Ndbl,int Ndwl,
            double Nspd,double NSubbanks,int RWP,int ERP,int EWP,int NSER,
            double techscaling_factor);
        area_type tagsubblock (int baddr,int C,int B,int A,int Ndbl,int Ndwl,
            double Nspd,double NSubbanks,int SB,int RWP,int ERP,int EWP,int NSER,
            double techscaling_factor);
        area_type tagarray (int baddr,int C,int B,int A,int Ndbl,int Ndwl,double Nspd,
            double NSubbanks,int RWP,int ERP,int EWP,int NSER,double techscaling_factor);
        void area (int baddr,int b0,int Ndbl,int Ndwl,double Nspd,int Ntbl,
            int Ntwl,int Ntspd,double NSubbanks,parameter_type *parameters,
            arearesult_type *result);
        area_type fadecode_row (int C,int B,int Ndbl,int RWP,int ERP,int EWP);
        area_type fasubarray (int baddr,int C,int B,int Ndbl,int RWP,int ERP,
            int EWP,int NSER,double techscaling_factor);
        area_type faarea (int baddr,int b0,int C,int B,int Ndbl,int RWP,int ERP,
            int EWP,int NSER,double techscaling_factor);
        void fatotalarea (int baddr,int b0,int Ndbl,parameter_type *parameters,arearesult_type *faresult);
        void area_subbanked (int baddr,int b0,int RWP,int ERP,int EWP,int Ndbl,
            int Ndwl,double Nspd,int Ntbl,int Ntwl,int Ntspd,double NSubbanks,
            parameter_type *parameters,area_type *result_subbanked,arearesult_type *result);
        int data_organizational_parameters_valid (int B,int A,int C,int Ndwl,int Ndbl,
            double Nspd,char assoc,double NSubbanks);
        int tag_organizational_parameters_valid (int B,int A,int C,int Ntwl,int Ntbl,
            int Ntspd,char assoc,double NSubbanks);
        
};


#endif

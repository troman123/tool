//filename mp4_boxes.cpp

//http://www.360doc.com/content/10/0802/15/474846_43156144.shtml  seek sample
//http://www.360doc.com/content/13/0627/14/6979751_295891383.shtml
//get mp4 file info http://www.360doc.com/content/10/1117/14/3779243_70131859.shtml
// #include <iostream>
// #include <fstream>
//#include <string>

#include <string.h>
#include <stdlib.h>
#include <error.h>

#include "mp4parser.h"

#ifndef RELEASE
#define RELEASE 1
#endif

#ifndef VARIABLES
#define VARIABLES                               \
    int k = 0;                                  \
    unsigned char p[5];                         \
    int inner_size = 0;
#endif

static struct mp4_moov_box moov;

/*final box
 */
int mp4_read_mvhd_box(int f, int size)  //level 3
{
    printf("\t+%s\n", "mvhd");
    VARIABLES;
    int level_4_box_size    = 0;

    mp4_mvhd_box mvhd;
    memset(&mvhd, 0, sizeof(mvhd));
    
    mvhd.size               = size;
    mvhd.type               = ('m' | 'v'<<8 | 'h'<<16 | 'd'<<24);
    mvhd.version            = read_uint8(f);                 //1


    readn(f, mvhd.flags, sizeof(mvhd.flags));
    //fread(mvhd.flags, sizeof(mvhd.flags), 1, f);             //3
    mvhd.creation_time      = read_uint32_lit(f);            //4
    mvhd.modification_time  = read_uint32_lit(f);            //4
    mvhd.timescale          = read_uint32_lit(f);            //4
    mvhd.duration           = read_uint32_lit(f);            //4
    mvhd.rate               = read_uint32_lit(f);            //4
    mvhd.volume             = read_uint16_big(f);            //2
    readn(f, &mvhd.reserved1, sizeof(mvhd.reserved1));    //2
    readn(f, &mvhd.reserved2, sizeof(mvhd.reserved2));    //8
    readn(f, mvhd.matrix, sizeof(mvhd.matrix));           //36
    readn(f, mvhd.pre_defined, sizeof(mvhd.pre_defined)); //24
    mvhd.next_track_id      = read_uint32_lit(f);            //4
    moov.mvhd               = mvhd;

    
    printf("\t\ttimescale: %u\n", moov.mvhd.timescale);
    printf("\t\tduration: %u\n", moov.mvhd.duration);
    printf("\t\trate: %u\n", moov.mvhd.rate );
    printf("\t\tvolume: 0x%x\n", moov.mvhd.volume);
    
    printf("\t\tmatrix:\n");
    int i = 0;
    for(i = 0; i < 3; ++i){
        printf("\t\t");
        int j = 0;
        for(j = 0; j < 3; ++j){
            printf(" %8u ", moov.mvhd.matrix[i*3+j]);
        }
        printf("\n");
    }
    
    printf("\t\tnext track id: %u\n", moov.mvhd.next_track_id);
   
    printf("\n");
}


/*final box
 */
struct mp4_tkhd_box mp4_read_tkhd_box(int f, int size)  //level 4
{
    printf("\t\t+%s\n", "tkhd"); 
    
    VARIABLES;
    mp4_tkhd_box box;
        
    box.size              = size;
    box.type              = 0;
    box.version           = read_uint8(f);

    readn(f, box.flags, sizeof(box.flags));
    //fread(&box.flags, sizeof(box.flags), 1, f);
    box.creation_time     = read_uint32_lit(f);
    box.modification_time = read_uint32_lit(f);
    box.track_ID          = read_uint32_lit(f);
    box.reserved1         = read_uint32_lit(f);
    box.duration          = read_uint32_lit(f);
    readn(f, &box.reserved2, sizeof(box.reserved2));
    box.layer             = read_uint16_big(f);
    box.alternate_group   = read_uint16_big(f);
    box.volume            = read_uint16_big(f);
    box.reserved3         = read_uint16_big(f);
    readn(f, &box.matrix, sizeof(box.matrix));
    box.width             = read_uint32_lit(f);
    box.height            = read_uint32_lit(f);

    // printf("\t\t\tflags: 0x%4x\n", box.flags[2]
    //        | box.flags[1] | box.flags[0]);
    // printf("\t\t\tcreation time: %u\n", box.creation_time);
    // printf("\t\t\tmodifaction time: %u\n",
    //        box.modification_time);
    printf("\t\t\ttrack id: %u\n", box.track_ID);
    // printf("\t\t\treserved1: 0x%x\n", box.reserved1);
    printf("\t\t\tduration: %d\n",box.duration);
    // printf("\t\t\treserved2: 0x%x, 0x%x\n",
    //        box.reserved2[0], box.reserved2[1]);
    printf("\t\t\tlayer: %d\n",box.layer);
    printf("\t\t\talternate group: %d\n", box.alternate_group);
    printf("\t\t\tvolume: 0x%x\n", box.volume);
    // printf("\t\t\treserved3: 0x%x\n", box.reserved3);
        
    printf("\t\t\tmatrix:\n");
    int i = 0;
    for(i = 0; i < 3; ++i){
        printf("\t\t\t");
        int j = 0;
        for(j = 0; j < 3; ++j){
            printf(" %8u ", box.matrix[i*3+j]);
        }
        printf("\n");
    }
    //////////////////////////////////////////@a mark still don't know
    printf("\t\t\twidth: %u\n",box.width);
    printf("\t\t\theight: [%u].[%u]\n",
           box.height & 0xffff0000 >> 16,
           box.height & 0xffff);
  
    printf("\n");
    
    return box;
}

/*container box
  (elst)
 */
struct mp4_edts_box mp4_read_edts_box(FILE *f, int size)
{
    printf("\t\t+%s\n", "edts");
    
    //return mp4_edts_box();
}

/*final box
 */
struct mp4_mdhd_box mp4_read_mdhd_box(int f, int size)
{
    printf("\t\t\t+mdhd\n");
    struct mp4_mdhd_box box;

    VARIABLES;
        
    box.size              = size;
    box.type              = 0;
    box.version           = read_uint8(f);

    readn(f, box.flags, sizeof(box.flags));
    //fread(&box.flags, sizeof(box.flags), 1, f);
    box.creation_time     = read_uint32_lit(f);
    box.modification_time = read_uint32_lit(f);
    box.timescale         = read_uint32_lit(f);
    box.duration          = read_uint32_lit(f);
    box.language          = read_uint16_big(f);
    box.pre_defined       = read_uint16_big(f);


    // printf("\t\t\t\t\tflags: 0x%x\n", box.flags[2]
    //        | box.flags[1] | box.flags[0]);
    // printf("\t\t\t\t\tcreation time: %u\n", box.creation_time);
    // printf("\t\t\t\t\tmodifaction time: %u\n",
    //        box.modification_time);
    printf("\t\t\t\ttimescale: %u\n", box.timescale);
    printf("\t\t\t\tduration: %u\n", box.duration);
    printf("\t\t\t\tlanguage: %u\n", box.language);
    // printf("\t\t\t\tpre-defined: %u\n", box.pre_defined);
    
//    printf("\n");

    return box;
}

/*filnal box
 */
struct mp4_hdlr_box mp4_read_hdlr_box(int f, int size)
{
    printf("\t\t\t%s\n", "+hdlr");
    struct mp4_hdlr_box box;
   
    VARIABLES;
    
    box.size            = size;
    box.type            = 'h'|'d'<<8|'l'<<16|'r'<<24;
    box.version         = read_uint8(f);

    readn(f, box.flags, sizeof(box.flags));
    //fread(&box.flags, sizeof(box.flags), 1, f);
    box.pre_defined     = read_uint32_lit(f);
    readn(f, &box.handler_type, sizeof(box.handler_type));
    box.handler_type[4] = 0;
    readn(f, &box.reserved, sizeof(box.reserved));
    readn(f, &inner_size, 1);
    box.name            = malloc((inner_size + 1) * sizeof(char));//new char[inner_size + 1];
    int i = 0;
    for(i = 0; i < inner_size; ++i)
        readn(f, &box.name[i], sizeof(unsigned char));
    
    // printf("\t\t\t\tflags: 0x%x\n",
    //        box.flags[2] | box.flags[1] | box.flags[0]);
    // printf("\t\t\t\tpre-defined: %d\n", box.pre_defined);
    printf("\t\t\t\thandler type: %s\n", box.handler_type);
    // printf("\t\t\t\treserved: 0x%x, 0x%x, 0x%x\n",
    //        box.reserved[0], box.reserved[1], box.reserved[2]);
    printf("\t\t\t\tname: %s\n", box.name);  //I have no idea what
                                             //is this using for
    // printf("\n");
    return box;
}

/*final box
 */
struct mp4_vmhd_box * mp4_read_vmhd_box(int f, int size)
{
    printf("\t\t\t\t+%s\n", "vmhd");

    VARIABLES;
    mp4_vmhd_box *box  = malloc(sizeof(mp4_vmhd_box));//new mp4_vmhd_box;
    
    box->size          = size;
    box->type          = 0;
    box->version       = read_uint8(f);
    readn(f, box->flags, sizeof(box->flags));
    //fread(&(box->flags), sizeof(box->flags), 1, f);
    box->graphics_mode = read_uint32_lit(f);
    readn(&(box->opcolor), sizeof(box->opcolor));
        

    // printf("\t\t\t\t\tflags: 0x%x\n", box.flags[2]
    //        | box.flags[1] | box.flags[0]);
    printf("\t\t\t\t\tgraphics mode: %u\n", box->graphics_mode);
    printf("\t\t\t\t\topcolor: %u,%u,%u,%u\n",
           box->opcolor[0],  box->opcolor[1],
           box->opcolor[2],  box->opcolor[3]);
    
    //printf("\n");
}

/*final box
 */
struct mp4_smhd_box *mp4_read_smhd_box(int f, int size)
{
    printf("\t\t\t\t+%s\n", "smhd");
    mp4_smhd_box box;
    
    box.size             = size;
    box.type             = 's'|'m'<<8|'h'<<16|'d'<<24;
    box.version          = read_uint8(f);

    readn(f, box.flags, sizeof(box.flags));
    //fread(&box.flags, sizeof(box.flags), 1, f);
    box.balance          = read_uint16_big(f);
    box.reserved         = read_uint16_big(f);        

    // printf("\t\t\t\t\tflags: 0x%x\n", box.flags[2]
    //        | box.flags[1] | box.flags[0]);
    printf("\t\t\t\t\tbalance: %d,%d\n",
           box.balance & 0xff00 >> 8, box.balance & 0xff);
    // printf("\t\t\t\t\treserved: 0x%x\n", box.reserved);
        
    //  printf("\n");
    mp4_smhd_box *newbox = malloc(sizeof(mp4_smhd_box));//new mp4_smhd_box;
    *newbox              = box;
    return newbox;
}

struct mp4_stts_box mp4_read_stts_box(int f, int size)  //level 8
{
    printf("\t\t\t\t\t+%s\n", "stts");
    mp4_stts_box box;

    box.size                 = size;
    box.version              = read_uint8(f);

    readn(f, box.flags, sizeof(box.flags));
    //fread(box.flags, sizeof(box.flags), 1, f);
    box.number_of_entries    = read_uint32_lit(f);

    // printf("\t\t\t\t\t\tflags: %u\n",
    //        box.flags[0]|box.flags[1]|box.flags[2]);
    printf("\t\t\t\t\t\tnumber of entries: %u  box size=%d\n",box.number_of_entries, size);
     
    printf("\t\t\t\t\t\tentries:\n\t\t\t\t\t\t");
    //box.time_to_sample_table = new uint32_t[box.number_of_entries];
    int sample_count = 0;
    int sample_duration = 0;
    int i = 0;
    for(i = 0; i < box.number_of_entries; ++i){
        box.time_to_sample_table = malloc(sizeof(mp4_stts_box_sample_duration));//new mp4_stts_box_sample_duration();
        box.time_to_sample_table[i].sample_count = read_uint32_lit(f);
        sample_count += box.time_to_sample_table[i].sample_count;
        box.time_to_sample_table[i].sample_duration = read_uint32_lit(f);
        sample_duration += box.time_to_sample_table[i].sample_duration;
        printf("time_to sample table sample_count=%u ", box.time_to_sample_table[i].sample_count);
        printf("time_to sample table sample_duration=%u ", box.time_to_sample_table[i].sample_duration);
        printf("\n");
    }
    //delete [] box.time_to_sample_table;
    
    printf("sample_counts =%d sample_durations=%d\n", sample_count, sample_duration);

    return box;
}

/*final box*/
struct mp4_avcC_box *mp4_read_avcC_box(FILE *f)
{
    struct mp4_avcC_box *ptr       = malloc(sizeof(mp4_avcC_box));//new mp4_avcC_box;
    
    ptr->size                      = read_uint32_lit(f);
    printf("avcC_size:%u\n",ptr->size);
    
    ptr->type                      = read_uint32_lit(f);
    if( ptr->type                  ==  ('a'<<24|'v'<<16|'c'<<8|'C')){
        printf("type:avcC\n");
    }
    
    ptr->configurationVersion      = read_uint8(f);
    printf("configurationVersion:%u\n", ptr->configurationVersion);
    ptr->AVCProfileIndication      = read_uint8(f);
    printf("AVCProfileIndication: %x\n", ptr->AVCProfileIndication);
    ptr->profile_compatibility     = read_uint8(f);
    printf("profile_compatibility: %x\n", ptr->profile_compatibility);
    ptr->AVCLevelIndication        = read_uint8(f);
    printf("AVCLevelIndication: %x\n", ptr->AVCLevelIndication);
    ptr->lengthSizeMinusOne        = 0x3 & read_uint8(f);  //2 bit
    printf("lengthSizeMinusOne:%u\n", ptr->lengthSizeMinusOne);
    ptr->numOfSequenceParameterSet = 0x1F & read_uint8(f);  //5bit
    printf("numOfSequenceParameterSet:%x\n", ptr->numOfSequenceParameterSet);
    ptr->sps                       = malloc(sizeof(struct SPS) * ptr->numOfSequenceParameterSet);//new mp4_avcC_box::SPS[ptr->numOfSequenceParameterSet];
    
    int i = 0;
    for(i = 0; i < ptr->numOfSequenceParameterSet; ++i ){
        ptr->sps[i].sequenceParameterSetLength = read_uint16_lit(f);
        printf("sequenceParameterSetLength: %u\n", ptr->sps[i].sequenceParameterSetLength);
        
        ptr->sps[i].sequenceParameterSetNALUnit = malloc(sizeof(uint8_t) * ptr->sps[i].sequenceParameterSetLength);
            //new uint8_t[ptr->sps[i].sequenceParameterSetLength];
        readn(f, (ptr->sps[i].sequenceParameterSetNALUnit),
              (ptr->sps[i].sequenceParameterSetLength));
        
        int j =0;
        for(j = 0; j < ptr->sps[i].sequenceParameterSetLength; ++j){
            printf("%02x", ptr->sps[i].sequenceParameterSetNALUnit[j]);
        }
        printf("============\n");
    }

    ptr->numOfPictureParameterSets = read_uint8(f);
    printf("numOfPictureParameterSets:%u\n", ptr->numOfPictureParameterSets);
    ptr->pps                       = malloc(sizeof(struct PPS) * ptr->numOfPictureParameterSets);//new mp4_avcC_box::PPS[ptr->numOfPictureParameterSets];
    //int i = 0;
    for(i = 0; i < ptr->numOfPictureParameterSets; ++i){
        ptr->pps[i].pictureParameterSetLength = read_uint16_lit(f);
        printf("%d\n", ptr->pps[i].pictureParameterSetLength);
        
        ptr->pps[i].pictureParameterSetNALUnit
                                   = malloc(sizeof(uint8_t) * ptr->pps[i].pictureParameterSetLength);//new uint8_t[ptr->pps[i].pictureParameterSetLength];
        
        readn(f, ptr->pps[i].pictureParameterSetNALUnit,
              ptr->pps[i].pictureParameterSetLength);
        
        int j = 0;
        for(j = 0; j < ptr->pps[i].pictureParameterSetLength; ++j){
            printf("%02x", ptr->pps[i].pictureParameterSetNALUnit[j]);
        }
        printf("============\n");
    }
    
    return ptr;
}

/*final box
  @a special contain a avcC box
 */
struct mp4_avc1_box* mp4_read_avc1_box(int f, int size)  //level 9
{
    mp4_avc1_box *ptr = malloc(sizeof(mp4_avc1_box));//new mp4_avc1_box;
    
    ptr->size                   = size;
    ptr->type                   = (('a'<<24)|('v'<<16)|('c'<<8)|('1'));
    
    readn(f, ptr->reserved, sizeof(ptr->reserved));
    //fread(ptr->reserved, sizeof(ptr->reserved), 1, f);  //6
    ptr->data_reference_index   = read_uint16_lit(f);   //2
    ptr->pre_defined            = read_uint16_big(f);   //2
    ptr->reserved1              = read_uint16_big(f);   //2
    readn(f, ptr->pre_defined1, sizeof(ptr->pre_defined1));  //3*4
    ptr->width                  = read_uint16_lit(f);   //2
    ptr->height                 = read_uint16_lit(f);   //2
    ptr->horiz_res              = read_uint32_lit(f);   //4
    ptr->vert_res               = read_uint32_lit(f);   //4
    ptr->reserved2              = read_uint32_lit(f);   //4
    ptr->frames_count           = read_uint16_lit(f);   //2
    readn(f, ptr->compressor_name, sizeof(ptr->compressor_name), 1, f);  //33
    ptr->bit_depth              = read_uint8(f);        //1
    ptr->pre_defined2           = read_uint16_big(f);   //2

    //avcC --AVCDecoderConfigurationRecord
    printf("-------------------------------------\n");
    printf("data_reference_index:%x\n", ptr->data_reference_index);
    printf("width:%u\n", ptr->width);
    printf("height:%u\n", ptr->height);
    printf("frames_count:%x\n", ptr->frames_count);
    printf("bit_depth:%u\n", ptr->bit_depth);
    printf("pre_defined2: %x\n", ptr->pre_defined2);
    
    ptr->avcC = mp4_read_avcC_box(f);
    
    printf("-------------------------------------\n");
    return ptr;
}

/*final box
 */
struct mp4_mp4a_box * mp4_read_mp4a_box(FILE*f, int size)  //level 9
{
    printf("\t\t\t\t\t\t+mp4a\n");
    return 0;
}

/*container box
  (mp4a,avc1)
 */
struct mp4_stsd_box mp4_read_stsd_box(int f, int size)  //level 8
{
    printf("\t\t\t\t\t+%s\n", "stsd");
    mp4_stsd_box box;

    box.size               = size;
    box.version            = read_uint8(f);
    readn(f, &box.flags, sizeof(box.flags));
    //fread(box.flags, sizeof(box.flags), 1, f);

    box.number_of_entries  = read_uint32_lit(f);

    // printf("\t\t\t\t\t\tflags: %u\n",
    //        box.flags[0]|box.flags[1]|box.flags[2]);
    printf("\t\t\t\t\t\tnumber of entries: %u\n", box.number_of_entries);
    
    box.sample_description = malloc(sizeof(Sample_description) * box.number_of_entries);//new Sample_description[box.number_of_entries];
    int i = 0;
    for(i =0; i < box.number_of_entries; ++i){
        box.sample_description[i].size = read_uint32_lit(f);
        box.sample_description[i].type = read_uint32_lit(f);
        
        INFO("sample_description %s\n", (char*)&(box.sample_description[i].type));
        INFO("sample_description %c\n", ((char*)&(box.sample_description[i].type))[0]);
        INFO("sample_description %c\n", ((char*)&(box.sample_description[i].type))[1]);
        INFO("sample_description %c\n", ((char*)&(box.sample_description[i].type))[2]);
        INFO("sample_description %c\n", ((char*)&(box.sample_description[i].type))[3]);

        if( box.sample_description[i].type == (('m'<<24)|('p'<<16)|('4'<<8)|('a'))){
            box.sample_description[i].mp4a
                = mp4_read_mp4a_box(f, box.sample_description[i].size);
            box.sample_description[i].avc1 = 0;
            
        }else if(box.sample_description[i].type
                 == (('a'<<24)|('v'<<16)|('c'<<8)|('1'))){
            box.sample_description[i].avc1
                = mp4_read_avc1_box(f, box.sample_description[i].size);
            box.sample_description[i].mp4a = 0;
        } 
    }

    return box;
}

/*final box
 */
struct mp4_stss_box mp4_read_stss_box(int f, int size)  //level 8
{
    printf("\t\t\t\t\t+%s\n", "stss");
    mp4_stss_box box;
    box.size              = size;
    box.version           = read_uint8(f);
    //fread(box.flags, sizeof(box.flags), 1, f);
    readn(f, &box.flags, sizeof(box.flags));
    box.number_of_entries = read_uint32_lit(f);
    
    // printf("\t\t\t\t\t\t\tflags: %u\n",
    //        box.flags[0]|box.flags[1]|box.flags[2]);
    printf("\t\t\t\t\t\tnumber of entries: %u\n",box.number_of_entries);

    printf("\t\t\t\t\t\tentries:\n");
    box.sync_sample_table = malloc(sizeof(uint32_t) * box.number_of_entries);//new uint32_t[box.number_of_entries];
    int i = 0;
    for(i =0; i < box.number_of_entries; ++i){
        box.sync_sample_table[i] = read_uint32_lit(f);
         printf("%6u ", box.sync_sample_table[i]);
         if( (i+1)%12 == 0) printf("\n");
    }
    // delete box.sync_sample_table;
    printf("\n");

    return box;
}

/*final box
 */
struct mp4_stsc_box mp4_read_stsc_box(int f, int size)  //level 8
{
    printf("\t\t\t\t\t+%s\n", "stsc");

    mp4_stsc_box box;
    
    box.version    = read_uint8(f);
    //fread(&box.flags, sizeof(box.flags), 1, f);
    readn(f, &box.flags, sizeof(box.flags));
    box.map_amount = read_uint32_lit(f);
   
    printf("\t\t\t\t\t\tmap-amount: %u\n", box.map_amount);
    
    box.scmap      = malloc(sizeof(mp4_list_t) * box.map_amount);//new mp4_list_t[box.map_amount];
    printf("\t\t\t\t\t\tfirst trunk:\tsamples-per-thunk:\tsample-description-ID\n");
    
    int i = 0;
    for(i = 0; i < box.map_amount; ++i){
        box.scmap[i].first_chunk_num = read_uint32_lit(f);
        box.scmap[i].sample_amount_in_cur_table = read_uint32_lit(f);
        box.scmap[i].sample_description_id = read_uint32_lit(f);
        
         printf("\t\t\t\t\t\t%13d", box.scmap[i].first_chunk_num);
         printf("\t%13d", box.scmap[i].sample_amount_in_cur_table);
         printf("\t%13d\n", box.scmap[i].sample_description_id);
    }

    int chunkNum = 0;
    int samplesCount = 0;
    //int i = 0;
    for(i = 0; i < box.map_amount - 1; ++i)
    {
        chunkNum = box.scmap[i + 1].first_chunk_num - box.scmap[i].first_chunk_num;
        samplesCount += box.scmap[i].sample_amount_in_cur_table * chunkNum;
        
    }

        samplesCount += box.scmap[box.map_amount - 1].sample_amount_in_cur_table;
    printf("\t\t\t\t\tsamplesCount = %d\n", samplesCount);
    //delete box.scmap;
    return box;
}

/*final box
 */
struct mp4_stsz_box mp4_read_stsz_box(int f, int size)  //level 8
{
    printf("\t\t\t\t\t+%s\n", "stsz");
    mp4_stsz_box box;

    box.size                 = size;
    box.version              = read_uint8(f);
    //fread(box.flags, sizeof(box.flags), 1, f);
    readn(f, box.flags, sizeof(box.flags));
    box.samples_size_intotal = read_uint32_lit(f);
    
    if(box.samples_size_intotal == 0){
        box.table_size = read_uint32_lit(f);
        box.sample_size_table = malloc(sizeof(uint32_t) * box.table_size);//new uint32_t[box.table_size];
        // printf("\t\t\t\t\t\tflags: 0x%x\n",
        //        box.flags[0]|box.flags[1]|box.flags[2]);
        printf("\t\t\t\t\t\tall samples amount: %u\n", box.samples_size_intotal);
        printf("\t\t\t\t\t\tsample table size: %u\n", box.table_size);
        
        printf("\t\t\t\t\t\tsmple_size_table box.table_size=%d:\n", box.table_size);
        long duration = 0;
        int i = 0;
        for(i = 0; i < box.table_size; ++i){
            box.sample_size_table[i] = read_uint32_lit(f);
            
            duration += box.sample_size_table[i];
            printf("%8u ", box.sample_size_table[i]);
            if((i+1) % 10 == 0) printf("\n");
        }
        printf(".............. samples size tables .............. %ld\n", duration);
    }
    
    return box;
}

void mp4_read_ctts_box(FILE *f, int size)
{
    printf("\t\t\t\t\t+%s\n", "ctts");
    
    uint8_t version;
    uint8_t flags[3];

    version              = read_uint8(f);
    //fread(flags, sizeof(flags), 1, f);
    readn(f, flags, sizeof(flags));
    uint32_t number_of_entries    = read_uint32_lit(f);

    // printf("\t\t\t\t\t\tflags: %u\n",
    //        box.flags[0]|box.flags[1]|box.flags[2]);
    printf("\t\t\t\t\t\tnumber of entries: %u  box size=%d\n",number_of_entries, size);
     
    int sample_counts = 0;
    int sample_durations = 0;

    printf("\t\t\t\t\t\tentries:\n\t\t\t\t\t\t");
    //box.time_to_sample_table = new uint32_t[box.number_of_entries];

    int i = 0;
    for(i = 0; i < number_of_entries; ++i){
        uint32_t sample_count = read_uint32_lit(f);
        uint32_t sample_duration = read_uint32_lit(f);
        sample_counts += sample_count;
        sample_durations += sample_duration;
        printf(" time_to sample table %d sample_count=%u ", i, sample_count);
        printf(" time_to sample table %d sample_duration=%u ", i, sample_duration);
        printf("\n");
    }
    //delete [] box.time_to_sample_table;
    
    printf("sample_counts =%d sample_durations=%d\n", sample_counts, sample_durations);
}
/*final box
 */
struct mp4_stco_box mp4_read_stco_box(int f, int size)
{
    printf("\t\t\t\t\t+%s\n", "stco");
    mp4_stco_box box;

    box.version                      = read_uint8(f);
    //fread(box.flags, sizeof(box.flags), 1, f);
    readn(f, box.flags, sizeof(box.flags));
    box.chunk_offset_amount          = read_uint32_lit(f);

    printf("\t\t\t\t\t\tflags: 0x %x\n",
           box.flags[0]|box.flags[1]|box.flags[2]);
    printf("\t\t\t\t\t\tchunk offest amount: %u\n",
           box.chunk_offset_amount);

    printf("\t\t\t\t\t\tchunk offset:\n");
    box.chunk_offset_from_file_begin = malloc(sizeof(uint32_t) * box.chunk_offset_amount);//new uint32_t[box.chunk_offset_amount];
    int i = 0;
    for(i = 0 ; i < box.chunk_offset_amount; ++i) {
        box.chunk_offset_from_file_begin[i] = read_uint32_lit(f);
        printf("%8x ", box.chunk_offset_from_file_begin[i]);
        if((i+1) % 10 == 0) printf("\n");
    }
//    delete [] box.chunk_offset_from_file_begin;

    return box;
}

/*container box
  (stsd, stts, stsz|stz2, stsc, stco|co64, ctts, stss)
*/
struct mp4_stbl_box mp4_read_stbl_box(FILE *f, int size)
{
    printf("\t\t\t\t+stbl\n");
    
    mp4_stbl_box box;
    
    VARIABLES;
    int box_size = 0;
    //int cur_pos = ftell(f);
    do {
        //fseek(f, cur_pos, SEEK_SET);
        
        box_size = read_uint32_lit(f);
        readn(f, p, 4);
        p[4]              = 0;
        
        //std::string name  = (char*)p;
        printf("\t\t\t\t+name=%s\n", p);

        if(strcmp(p, "stsd") == 0) {
            box.stsd      = mp4_read_stsd_box(f, box_size);
            //根据不同的编码方案和存储数据的文件数目，
            //每个media可以有一个到多个sample description。
            //sample-to-chunk atom通过这个索引表，
            //找到合适medai中每个sample的description。
            
        } else if(strcmp(p, "stts") == 0){
            box.stts      = mp4_read_stts_box(f, box_size);
            //Time-to-sample atoms存储了media sample的duration 信息，
            //提供了时间对具体data sample的映射方法，通过这个atom，
            //你可以找到任何时间的sample，类型是'stts'。
            
        } else if(strcmp(p, "stss") == 0){
            box.stss      = mp4_read_stss_box(f, box_size);
            //sync sample atom确定media中的关键帧。
        } else if(strcmp(p, "stsc") == 0){
            box.stsc      = mp4_read_stsc_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7922.html
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7923.html
            
        } else if(strcmp(p, "stsz") == 0 || strcmp(p, "stz2") == 0){
            box.stsz      = mp4_read_stsz_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7924.html
            
        } else if(strcmp(p, "stco") == 0 || strcmp(p, "c064") == 0){
            box.stco      = mp4_read_stco_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7925.html
            
        } else if(strcmp(p, "ctts") == 0) {
            //@a undefined
            mp4_read_ctts_box(f, box_size);
        } else {
            //printf("\t\t====size: %u\n", box_size);
            printf("\t\t\t\t\t+%s===============mark undifined\n", p);
        }
        
        //cur_pos    += box_size;
        inner_size += box_size;
    } while(inner_size+8 != size);
    
    //printf("\n");
    return box;
}

/*container box
  ()
 */
struct mp4_dinf_box mp4_read_dinf_box(FILE *f, int size)
{
    printf("\t\t\t\t+dinf\n");
    struct mp4_dinf_box box;

    return box;
}

/*container box
  (vmhd, smhd, hmhd, nmhd)
*/
struct mp4_minf_box mp4_read_minf_box(FILE *f, int size)
{
    struct mp4_minf_box box;
    
    printf("\t\t\t+%s\n", "minf");

    VARIABLES;
    int level_5_box_size = 0;
    //int cur_pos          = ftell(f) ;
    
    do{
        //fseek(f, cur_pos, SEEK_SET);
        
        level_5_box_size = read_uint32_lit(f);
        readn(f, p, 4);
        p[4] = 0;
        
        //std::string name = (char*)p;
        if(strcmp(p, "vmhd") == 0) {
            box.vmhd = mp4_read_vmhd_box(f, level_5_box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7913.html
            
        } else if(strcmp(p, "dinf") == 0) {
            box.dinf = mp4_read_dinf_box(f, level_5_box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7915.html
            
        } else if(strcmp(p, "stbl") == 0) {
            box.stbl = mp4_read_stbl_box(f, level_5_box_size);
            //@a unfind
            
        }else if(strcmp(p, "smhd") == 0){
            box.smhd = mp4_read_smhd_box(f, level_5_box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7914.html
            
        } else {
            printf("\t\t====size: %u\n", level_5_box_size);
            printf("\t\t====type: %s\n", p);
        }
        
        //cur_pos    += level_5_box_size;
        inner_size += level_5_box_size;
    } while(inner_size+8 != size);
    
    printf("\n");
    return box;
}

/*container box
  (mdhd, hdlr, minf)
*/
struct mp4_mdia_box mp4_read_mdia_box(FILE *f, int size)
{
    printf("\t\t+%s\n", "mdia");
    
    mp4_mdia_box box;
    box.size              = size;
    box.type              = 'm' | 'd'<<8 | 'i'<<16 | 'a'<<24;
    VARIABLES;
    int box_size          = 0;
    //int cur_pos           = ftell(f) ;
    do{
        //fseek(f, cur_pos, SEEK_SET);
        
        box_size          = read_uint32_lit(f);
        readn(f, p, 4);
        p[4]              = 0;
        
        //std::string name  = (char*)p;
        if(strcmp(p, "mdhd") == 0) {
            box.mdhd      = mp4_read_mdhd_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7908.html
            
        } else if(strcmp(p, "hdlr") == 0) {
            box.hdlr      = mp4_read_hdlr_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7909.html
            
        }else if(strcmp(p, "minf") == 0){
            box.minf      = mp4_read_minf_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7910.html
            
        } else {
            //printf("\t\t====size: %u\n", box_size);
            printf("\t\t====type: %s\n", p);
        }
        
        //cur_pos    += box_size;
        inner_size += box_size;
    } while(inner_size+8 != size);
    printf("\n");
    
    return box;
}

/*container box
 */
struct  mp4_udta_box mp4_read_udta_box(FILE *f, int size)
{
    printf("\t\t+%s\n", "udta");
    mp4_udta_box a;
    return a;
}

/* container box
   (tkhd, mdia)
*/
int mp4_read_trak_box(FILE *f, int size)
{
    printf("\t+%s\n", "trak");
    
    struct mp4_trak_box *trak  = malloc(sizeof(mp4_trak_box));//new struct mp4_trak_box;
    VARIABLES;
    int box_size               = 0;
    //int cur_pos                = ftell(f) ;
    do{
        //fseek(f, cur_pos, SEEK_SET);

        box_size               = read_uint32_lit(f);
        readn(f, p, 4);
        p[4]                   = 0;
        //std::string name       = (char*)p;
        if(strcmp(p, "tkhd") == 0)
        {
            trak->tkhd         = mp4_read_tkhd_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7903.html
            
        }
        else if(strcmp(p, "edts") == 0)
        {
            trak->edts         = mp4_read_edts_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7904.html
            
        } else if(strcmp(p, "mdia") == 0) {
            trak->mdia = mp4_read_mdia_box(f, box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7907.html

        } else if(strcmp(p, "udta") == 0){
            trak->udta = mp4_read_udta_box(f, box_size);
            //same as the level 3 box, @a special
            
        } else {
            //printf("\t====size: %u\n", box_size);
            printf("\t\t+%s===========\n", p);
        }
        
        //cur_pos    += box_size;
        inner_size += box_size;
    } while(inner_size+8 != size);

    //moov.trak.push_back(trak);
}

/*final box
 */
int mp4_read_iods_box(FILE *f, int size)  //level 3
{
    printf("\t+iods-------------------------undefined\n\n");
}

/*container box
  (mvhd, trak)
*/
int mp4_read_moov_box(int f, int size)   //level 2
{
    moov.size             = size;
    moov.type             = 'm'|'o'<<8|'o'<<16|'v'<<24;
    VARIABLES;
    int level_2_box_size  = 0;
    
    //int cur_pos           = ftell(f);
    printf("+%s size=%d \n",  "moov ", size);
    do {
        //fseek(f, cur_pos, SEEK_SET);
        
        level_2_box_size  = read_uint32_lit(f);
        
        //fread(&k, sizeof(k), 1, f);  //read byte
        readn(f, &k, sizeof(k));
        indian_a.size     = k;
        memcpy(p, indian_a.byte, sizeof(indian_a.byte));
        p[4]              = 0;
        
        //std::string name  = (char*)p;
        if(strcmp(p, "mvhd") == 0)
        {
            mp4_read_mvhd_box(f, level_2_box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7901.html
            
        } else if(strcmp(p, "trak") == 0)
        {
            mp4_read_trak_box(f, level_2_box_size);
            //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7902.html
            
        } else if(strcmp(p, "iods") == 0){
            mp4_read_iods_box(f, level_2_box_size);
            
        } else if(strcmp(p, "udta") == 0){
            mp4_read_udta_box(f, level_2_box_size);
            //@a mark parse it, unknow what it is    
        
        } else {
            printf("====%s\n\n", p);
        }
        
        //cur_pos    += level_2_box_size;
        inner_size += level_2_box_size;
    }while( inner_size+8 != size);
}

/*final box
 */
int mp4_read_ftyp_box(FILE *f, int size)  //level 2
{
    printf("+ftyp\n");
    
    size -= 8;
    char *ch = malloc(sizeof(char) * (size+1));//new char[size+1];
    int i = 0;
    for(i = 0; i < size; ++i) {
        readn(f, &ch[i], sizeof(char));
    }
    ch[size] = 0;
    
    printf("\tftyp: %s\n",  ch);
    //delete [] ch;
    free(ch);
}

/*@a special final box
 */
int mp4_read_mdat_box(int fd, int size)
{
    printf("\t+%s\n", "mdat");
    printf("\t\tthis is the real media data size=%d\n", size);
}

/*container box
  (ftyp, free, mdat, moov)
*/
int mp4_read_root_box(int f, int *curr_pos) //level 1
{
    int k                 = 0;
    unsigned char p[5];
    
    int level_1_box_size  = read_uint32_lit(f);
    *curr_pos += 4;
    printf("root size=%d\n", level_1_box_size); 
    // INFO("root name %d\n", ((char*)&(level_1_box_size))[0]);
    // INFO("root name %d\n", ((char*)&(level_1_box_size))[1]);
    // INFO("root name %d\n", ((char*)&(level_1_box_size))[2]);
    // INFO("root name %d\n", ((char*)&(level_1_box_size))[3]);

    //fread(&k, sizeof(k), 1, f);  //read byte
    readn(f, &k, sizeof(k));
    *curr_pos += sizeof(k);
    // INFO("root name %p\n", &((char*)&(k))[0]);
    // INFO("root name %p\n", &((char*)&(k))[1]);
    // INFO("root name %p\n", &((char*)&(k))[2]);
    // INFO("root name %p\n", &((char*)&(k))[3]);

    INFO("root name %c\n", ((char*)&(k))[0]);//f
    INFO("root name %c\n", ((char*)&(k))[1]);//y
    INFO("root name %c\n", ((char*)&(k))[2]);//t
    INFO("root name %c\n", ((char*)&(k))[3]);//p

    indian_a.size         = k;
    memcpy(p, indian_a.byte, sizeof(indian_a.byte));
    p[4]                  = 0;
    
    //std::string name = (char*)p;
    fprintf(stdout, "read root box: type=%s box_size=%d\n", p, level_1_box_size);

    *curr_pos += level_1_box_size;
    if(strcmp(p, "moov") == 0)
    {
        if (1)
        {
            return 0;
        }
        mp4_read_moov_box(f, level_1_box_size);

        //@a http://www.52rd.com/Blog/Detail_RD.Blog_wqyuwss_7900.html
        
    }
    else if(strcmp(p, "ftyp") == 0)
    {
        mp4_read_ftyp_box(f, level_1_box_size);
    }
    else if(level_1_box_size == 0)
    {
      //till the end of file
        return 0;
        
    }
    else if(strcmp(p, "mdat")  == 0)
    {
        if (1)
        {
            return -1;
        }
        //mp4_read_mdat_box(f, level_1_box_size);

    }
    else if(strcmp(p, "free") == 0)
    {
        printf("+free\n");
        
    }
    else
    {
        //printf("==%u\n", level_1_box_size);
        printf("%s==mark undifined\n", p);
    }
    
    return level_1_box_size;
}


void print_trak(const struct mp4_trak_box * trak)
{
    printf("trak\n");

    
    printf("\t+%s\n", "tkhd");
    printf("\t\ttrack id: %u\n", trak->tkhd.track_ID);
    printf("\t\tduration: %d\n", trak->tkhd.duration);
    printf("\t\t\tlayer: %d\n",trak->tkhd.layer);
    printf("\t\t\talternate group: %d\n", trak->tkhd.alternate_group);
    printf("\t\t\tvolume: 0x%x\n", trak->tkhd.volume);
    printf("\t\t\tmatrix:\n");

    int i = 0;
    for(i = 0; i < 3; ++i){
        printf("\t\t\t");
        int j = 0;
        for(j = 0; j < 3; ++j){
            printf(" %8u ", trak->tkhd.matrix[i*3+j]);
        }
        printf("\n");
    }
    printf("\t\t\twidth: %u\n",trak->tkhd.width);
    printf("\t\t\theight: [%u].[%u]\n",
           trak->tkhd.height & 0xffff0000 >> 16,
           trak->tkhd.height & 0xffff);


    
    printf("\t+%s\n", "mdia");
    
    printf("\t\t\t\t\t\t+stsd\n");
    printf("\t\t\t\t\t\t\tnumber of entries: %u\n",
           trak->mdia.minf.stbl.stsd.number_of_entries);
    
    // printf("\t\t\t\t\t\t+stts: time to sample\n");
    // printf("\t\t\t\t\t\t\tentries:\n\t\t\t\t\t\t\t");
    // for(int i =0; i < trak->mdia.minf.stbl.stts.number_of_entries; ++i){
    //     printf("%15u ", trak->mdia.minf.stbl.stts.time_to_sample_table[i]);
    // }
    // printf("\n");

    // printf("\t\t\t\t\t+stss：关键帧\n");
    // printf("\t\t\t\t\t\t\tentries:\n\t\t\t\t\t\t\t");
    // for(int i =0; i < trak->mdia.minf.stbl.stss.number_of_entries; ++i){
    //     printf("%15u ", trak->mdia.minf.stbl.stss.sync_sample_table[i]);
    // }
    // printf("\n");

    printf("\t\t\t\t\t\t+stsc:sample to chunk table\n");
    printf("\t\t\t\t\t\tfirst trunk:\tsamples-per-thunk:\tsample-description-ID\n");
    //int i = 0;
    for(i = 0; i < trak->mdia.minf.stbl.stsc.map_amount; ++i){
        printf("\t\t\t\t\t\t%13d", trak->mdia.minf.stbl.stsc.scmap[i].first_chunk_num);
        printf("\t%13d", trak->mdia.minf.stbl.stsc.scmap[i].sample_amount_in_cur_table);
        printf("\t%13d\n", trak->mdia.minf.stbl.stsc.scmap[i].sample_description_id);
    }   printf("\n");

    printf("\t\t\t\t\t\t+%s\n", "stsz");
    printf("\t\t\t\t\t\t\tsmple_size_table:\n\t\t\t\t\t\t");
    //int i = 0;
    for(i = 0; i < trak->mdia.minf.stbl.stsz.table_size; ++i){
        printf("%8u ", trak->mdia.minf.stbl.stsz.sample_size_table[i]);
    }   printf("\n");

    printf("\t\t\t\t\t\t+stco\n");
    printf("\t\t\t\t\t\tchunk offset:\n\t\t\t\t\t");
    //int i = 0;
    for(i = 0 ; i < trak->mdia.minf.stbl.stco.chunk_offset_amount; ++i){
        printf("%8u ",trak->mdia.minf.stbl.stco.chunk_offset_from_file_begin[i]);
    }
}

uint32_t get_sample_num_in_cur_chunk(const struct mp4_stsc_box box,
                                     const uint32_t chunk_index)  //begin from 0
{
    int sample_num_in_cur_chunk_ = 0;
    
    int i = 0;
    for(i = 0; i < box.map_amount; ++i) {
        if(i+1 == box.map_amount){
            //std::cout<<"chunk_index:"<<chunk_index<<std::endl;
                
            sample_num_in_cur_chunk_
                = box.scmap[i].sample_amount_in_cur_table;
        }

        if(chunk_index+1 >= box.scmap[i].first_chunk_num
           && chunk_index+1 < box.scmap[i+1].first_chunk_num){
            sample_num_in_cur_chunk_
                =  box.scmap[i].sample_amount_in_cur_table;
            
                 break;
        }
    }
    
    //std::cout<<"sample_num_in_cur_chunk_:"<< sample_num_in_cur_chunk_;
    
    return sample_num_in_cur_chunk_;
}

/*@a return index from 0*/
uint32_t get_sample_index(const struct mp4_stsc_box box,
                          const uint32_t chunk_index)  //[1,end)
{
    /*chunk   samples   id
      1         8        1
      2         7        1
      46        6        1
    */
    uint32_t me     = chunk_index;
    uint32_t sindex = 0;
    
    int i = 0;
    for(i = 0; i < box.map_amount; ++i) {
        uint32_t chunk_amount = 0;
        uint32_t curr         = box.scmap[i].first_chunk_num;
        
        if(i+1 == box.map_amount){ //end() , we can't have next one to index
            chunk_amount = me - curr;  //do not add ONE
            sindex       += box.scmap[i].sample_amount_in_cur_table * chunk_amount;
            break;
        }
        uint32_t next    = box.scmap[i + 1].first_chunk_num;
        if(me > curr){
            if(me < next){
                chunk_amount = me - curr;  //do not add ONE
                sindex       += box.scmap[i].sample_amount_in_cur_table * chunk_amount;
                break;
            }else{
                chunk_amount = next - curr;  //do not add ONE
                sindex       += box.scmap[i].sample_amount_in_cur_table * chunk_amount;
            }
        } else if(me == curr){
            break;
        }
    }
    return sindex;
}

uint32_t get_sample_size(const struct mp4_stsz_box box,
                         const uint32_t chunk_index)  //[0, end)
{
    int i = 0;
    for(i = 0; i < box.table_size; ++i) {
        if(chunk_index == i)
            return box.sample_size_table[i];
    }
}

void copy_chunk_data(FILE *fin, 
                     const struct mp4_stbl_box box,
                     const uint32_t chunk_index,  //[0, end)
                     FILE *fout)
{
    printf("copy chunk data offset=%d", box.stco.chunk_offset_from_file_begin[chunk_index]);
    fseek(fin, box.stco.chunk_offset_from_file_begin[chunk_index],
          SEEK_SET);
    
    uint32_t sample_num_in_cur_chunk_
        = get_sample_num_in_cur_chunk(box.stsc, chunk_index);  //@a mark
    uint32_t sample_index_
        = get_sample_index(box.stsc, chunk_index+1);

    printf("%x\tindex=%u\t",
           box.stco.chunk_offset_from_file_begin[chunk_index],
           sample_index_);

    int i = 0;
    for(i = 0; i < sample_num_in_cur_chunk_; ++i) {
        uint32_t sample_size_ = get_sample_size(box.stsz, sample_index_+i);
        //std::cout<<sample_size_<<std::endl;
        uint32_t NALU_size  = 0;
        uint32_t NALU_total = 0;
        
        while(NALU_total < sample_size_){
            NALU_size           = read_uint32_lit(fin);
            NALU_total          += NALU_size+4;
            //printf("\tNALU_size:%x  sample_size_:%x\n", NALU_size, sample_size_);
            
            char *ptr           = malloc(sizeof(char) * NALU_size);//new char [NALU_size];
            fread(ptr, NALU_size, 1, fin);
            
            static uint32_t one = 1 << 24;
            fwrite(&one, sizeof(one), 1, fout);
            fwrite(ptr, NALU_size, 1, fout);
            
            //delete [] ptr;
            free(ptr);
            NALU_size           = 0;
        }
    }
}

void make_file(FILE *fin, const struct mp4_trak_box *trak, const char *name)
{
    int chunk_offset_amount = trak->mdia.minf.stbl.stco.chunk_offset_amount;

#if RELEASE
    struct mp4_avc1_box * avc1 =
        trak->mdia.minf.stbl.stsd.sample_description[0].avc1;

    //name += "x.h264";
    char filename[1024];
    sprintf(filename, "%sx.h264", name);
    FILE *fout = fopen(filename, "w");
    if (!fout)
    {
        //std::string tmp = "fopen_" + name ;
        perror(filename);
    }

    if (avc1 != 0)
    {
        uint32_t one = 1 << 24;
        fwrite(&one, sizeof(one), 1, fout);
        
        struct mp4_avcC_box * avcC = avc1->avcC;
        fwrite(avcC->sps[0].sequenceParameterSetNALUnit,
               avcC->sps[0].sequenceParameterSetLength, 1, fout);
        fwrite(&one, sizeof(one), 1, fout);
        fwrite(avcC->pps[0].pictureParameterSetNALUnit,
               avcC->pps[0].pictureParameterSetLength, 1, fout);
    }

    int chunk_index = 0;
    for(chunk_index = 0 ; chunk_index < chunk_offset_amount; ++chunk_index)
    {
        copy_chunk_data(fin, trak->mdia.minf.stbl, chunk_index, fout);
    }

    fclose(fout);
#else
    
    system(std::string("rm -rf " + name).c_str());
    system(std::string("mkdir "  + name).c_str());
    
    int chunk_index = 0;
    for(chunk_index = 0 ; chunk_index < chunk_offset_amount;
        ++chunk_index) {
        char char_num[10];
        sprintf(char_num, "%u", chunk_index);
        
        FILE *fout = fopen(std::string(name + "/" + name + char_num).c_str(), "w");
        
        if(fout == NULL)
        {
            printf("error\n");

            std::exit(-1);
        }
        
        copy_chunk_data(fin, trak->mdia.minf.stbl, chunk_index, fout);
        fclose(fout);
    }
#endif
}
#include <assert.h>
int main2(int argc, char **argv)
{
    //std::ios_base::sync_with_stdio(true);

    //std::string luo   = "luohunshidai01_001.mp4";
    //std::string _600  = "6004501011.mp4";
    
//    FILE *fin       = fopen(luo.c_str(), "r");
    // FILE *fin = fopen(argv[1], "r");
    // //FILE *f_video   = fopen("video", "w+");
    // //FILE *f_audio   = fopen("audio", "w+");
    
    // if(fin == (FILE*)0)
    // {
    //     printf("failed to open\n");
    //     return 0;
    // }
    
    // //level 0
    // int cur_pos = ftell(fin);
    // for(; !feof(fin); )
    // {
    //     fseek(fin, cur_pos, SEEK_SET);
    //     printf("----------------------------------------level 0 cur_pos=%d\n", cur_pos);
    //     cur_pos += mp4_read_root_box(fin);
    // }

    //print_trak(moov.trak[0]);
    //print_trak(moov.trak[1]);

    //make_file(fin, moov.trak[0], ("video"));
//    make_file(fin, moov.trak[1], ("audio"));

    return 0;
}

int parseMp4Box(int fd, int *type)
{
    // int cur_pos = ftell(fin);
    int curr_pos = 0;
    int ret = 0;
    *type = 0;
    while((ret = mp4_read_root_box(fd, &curr_pos)) != 0)
    {
        if (ret == -1)
        {
            *type = 1;
            break;
        }
    }


    fprintf(stdout, "parseMp4Box: curr_pos=%d\n", curr_pos);

    return curr_pos;
}


//filename mp4_boxes.h
#ifndef MP4_BOXES_H
#define MP4_BOXES_H

//#include <vector>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static union ____indian_swap
{
    unsigned char byte[4];
    unsigned int size;
} indian_a , indian_b;

unsigned read_uint16_big(int fd)
{
    unsigned short k = 0;
    readn(fd, &k, sizeof(k));
    return k;
}

uint16_t read_uint16_lit(int fd)
{
    uint16_t k;
    readn(fd, &k, sizeof(k));
    return ((k&0xff00) >> 8)|((k&0xff) << 8);
}

unsigned char read_uint8(int fd)
{
    unsigned char x;
    readn(fd, &x, sizeof(x));
    return x;
}

int read_uint32_lit(int fd)
{
    int k = 0;
    //fread(&k, sizeof(k), 1, f);
    readn(fd, &k, sizeof(k));
    indian_a.size = k;
    int i = 0;
    int j = 0;
    for(i = 0, j = 3; i < 4; i++, j--)
    {
        indian_b.byte[i] = indian_a.byte[j];
    }
    
    return indian_b.size;
}

#ifndef MP4_BASE_BOX
#define MP4_BASE_BOX                            \
    uint32_t type;                              \
    uint32_t  size;
#endif

#ifndef MP4_FULL_BOX
#define MP4_FULL_BOX                            \
    MP4_BASE_BOX;                               \
    uint8_t version;                            \
    uint8_t flags[3]
#endif

#ifndef MP4_SAMPLE_ENTRY_FIELDS
#define MP4_SAMPLE_ENTRY_FIELDS                    \
    MP4_BASE_BOX;                                \
    char reserved[6];                            \
    uint16_t data_reference_index;
#endif

#ifndef VISUAL_SAMPLE_ENTRY_FIELDS
#define VISUAL_SAMPLE_ENTRY_FIELDS                \
    MP4_SAMPLE_ENTRY_FIELDS;                    \
    uint16_t      pre_defined;                  \
    uint16_t      reserved1;                    \
    uint32_t      pre_defined1[3];              \
    uint16_t      width;                        \
    uint16_t      height;                       \
    uint32_t      horiz_res;                    \
    uint32_t      vert_res;                     \
    uint32_t      reserved2;                    \
    /**/uint16_t  frames_count;                 \
    /*以下是AVCDecoderConfigurationRecord*/     \
    /**/char      compressor_name[33];          \
    /**/uint16_t  bit_depth;                    \
    /**/int16_t   pre_defined2
#endif

typedef struct mp4_box{
    MP4_BASE_BOX;
} mp4_box;

typedef struct mp4_avcC_box{
    uint32_t      size;
    uint32_t      type;
    uint8_t       configurationVersion;      //=1
    uint8_t       AVCProfileIndication;
    uint8_t       profile_compatibility;
    uint8_t       AVCLevelIndication;
    
    uint8_t       lengthSizeMinusOne;        // & 0x3,  ==2 bit
    uint8_t       numOfSequenceParameterSet; // & 0x1F  ==5bit
    struct SPS{
        uint16_t  sequenceParameterSetLength;
        uint8_t   *sequenceParameterSetNALUnit;
    }             *sps;

    uint8_t       numOfPictureParameterSets;
    struct PPS{
        uint16_t  pictureParameterSetLength;
        uint8_t   *pictureParameterSetNALUnit;
    }             *pps; 
} mp4_avcC_box;


typedef struct mp4_avc1_box {
    VISUAL_SAMPLE_ENTRY_FIELDS;
    struct mp4_avcC_box *avcC;
    struct mp4_btrt_box *btrt;
    /*ext descriptors */
    struct mp4_m4ds_box *m4ds;
} mp4_avc1_box;

typedef struct mp4_tkhd_box {
    MP4_FULL_BOX;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t track_ID;
    uint32_t reserved1;
    uint32_t duration;

    uint32_t reserved2[2];
    uint16_t layer;
    uint16_t alternate_group;

    uint16_t volume;
    uint16_t reserved3;
    uint32_t matrix[9];
    uint32_t width;
    uint32_t height;
} mp4_tkhd_box;

typedef struct editList{
    uint32_t trak_duration;
    uint32_t duration;
    uint32_t rate;
} editList;

typedef struct mp4_elst_box{
    MP4_FULL_BOX;
    uint32_t amount;
    editList *edit_list;
} mp4_elst_box;

typedef struct mp4_mdhd_box {
    MP4_FULL_BOX;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t timescale;
    uint32_t duration;
    uint16_t language;
    uint16_t pre_defined;
} mp4_mdhd_box;

typedef struct mp4_vmhd_box{
    MP4_FULL_BOX;
    uint32_t graphics_mode;
    uint8_t opcolor[4];  //==============take care
} mp4_vmhd_box;

typedef struct mp4_dref_box{
    MP4_FULL_BOX;
    uint32_t entry_count;
    //there are many urls
    //======================================
} mp4_dref_box;

typedef struct mp4_list_t{
    uint32_t first_chunk_num;
    uint32_t sample_amount_in_cur_table;
    uint32_t sample_description_id;
} mp4_list_t;

typedef struct mp4_stsc_box{  //Sample-to-Chunk Atoms
    MP4_FULL_BOX;
    uint32_t map_amount;
    mp4_list_t *scmap; //sample-to-trunk表的结构
} mp4_stsc_box;

typedef struct mp4_stsz_box{  //sample size
    MP4_FULL_BOX;
    uint32_t samples_size_intotal;
    uint32_t table_size;  // 全部sample的数目。如果所有的sample有相同的长度，这个字段就是这个值。
                                // 否则，这个字段的值就是0。那些长度存在sample size表中
    uint32_t *sample_size_table;  //多个sample的大小相加就是整个电影的长度
} mp4_stsz_box;

typedef struct mp4_stco_box{  //Chunk Offset
    MP4_FULL_BOX;
    uint32_t chunk_offset_amount;
    uint32_t *chunk_offset_from_file_begin;
} mp4_stco_box;

typedef struct mp4_stss_box{  //Sync Sample 
    MP4_FULL_BOX;
    uint32_t number_of_entries;
    uint32_t *sync_sample_table;
} mp4_stss_box;

typedef struct mp4_stts_box_sample_duration {
    uint32_t sample_count;
    uint32_t sample_duration;
} mp4_stts_box_sample_duration;

typedef struct mp4_stts_box{  //Time-to-Sample
    MP4_FULL_BOX;
    uint32_t number_of_entries;
    mp4_stts_box_sample_duration *time_to_sample_table;
} mp4_stts_box;

#ifndef MP4_SAMPLE_ENTRY_FIELDS
#define MP4_SAMPLE_ENTRY_FIELDS                    \
    MP4_BASE_BOX;                                \
    char     reserved[6];                       \
    uint16_t data_reference_index;
#endif

typedef struct mp4_mp4a_box{
} mp4_mp4a_box;

typedef struct Sample_description{
    uint32_t            size;
    uint32_t            type;
    struct mp4_avc1_box *avc1;
    struct mp4_mp4a_box *mp4a;
    //利用这个索引可以检索与当前sample description关联的数据。
    //数据引用存储在data reference atoms。
} Sample_description;

typedef struct mp4_stsd_box{
    MP4_FULL_BOX;                           //12 bytes
    uint32_t            number_of_entries;  //4 bytes
    Sample_description *sample_description;
} mp4_stsd_box;

typedef struct mp4_smhd_box{  //sound media header box
    MP4_FULL_BOX;
    uint16_t            balance;
    uint16_t            reserved;
} mp4_smhd_box;

typedef struct mp4_hdlr_box {
    MP4_FULL_BOX;
    uint32_t       pre_defined;
    uint8_t        handler_type[5];
    uint32_t       reserved[3];
    char           *name;  //end with '\0'
} mp4_hdlr_box;

/* 14496-12 8.2, media data container */
typedef struct mp4_mdat_box {
    MP4_BASE_BOX;
    uint64_t data_size;
    char     *data;
} mp4_mdat_box;

typedef struct mp4_mvhd_box{
    MP4_FULL_BOX;                //12
    uint32_t creation_time;      //4
    uint32_t modification_time;  //4
    uint32_t timescale;          //4
    uint32_t duration;           //4
    uint32_t rate;               //4
    uint16_t volume;             //2
    uint16_t reserved1;          //2
    uint32_t reserved2[2];       //8
    uint32_t matrix[9];          //36
    uint32_t pre_defined[6];     //24
    uint32_t next_track_id;      //4
} mp4_mvhd_box;

typedef struct mp4_iods_box{
} mp4_iods_box;

typedef struct mp4_udta_box{
} mp4_udta_box;

typedef struct mp4_dinf_box{
} mp4_dinf_box;

typedef struct mp4_stbl_box{
    struct mp4_stsd_box stsd;
    struct mp4_stts_box stts;
    struct mp4_stss_box stss;
    struct mp4_stsc_box stsc;
    struct mp4_stsz_box stsz;
    struct mp4_stco_box stco;
} mp4_stbl_box;

typedef struct mp4_minf_box{
    MP4_BASE_BOX;
    struct mp4_vmhd_box *vmhd;
    struct mp4_smhd_box *smhd;
    struct mp4_hdlr_box hdlr;
    struct mp4_dinf_box dinf;
    struct mp4_stbl_box stbl;
} mp4_minf_box;

typedef struct mp4_mdia_box {
    MP4_BASE_BOX;
    struct mp4_mdhd_box mdhd;
    struct mp4_hdlr_box hdlr;
    struct mp4_minf_box minf;
} mp4_mdia_box;

typedef struct mp4_edts_box{
} mp4_edts_box;

typedef struct mp4_trak_box{
    struct mp4_tkhd_box tkhd;
    struct mp4_mdia_box mdia;
    struct mp4_edts_box edts;
    struct mp4_udta_box udta;
} mp4_trak_box;

typedef struct mp4_moov_box{
    MP4_BASE_BOX;
    struct mp4_mvhd_box mvhd;
    struct mp4_iods_box iods;
    // std::vector<struct mp4_trak_box *>
    //                     trak;
    struct mp4_trak_box ** traks;
    int traks_count;
    struct mp4_udta_box udta;
} mp4_moov_box;


int parseMp4Box(int fd, int *type);
#define INFO printf

#endif  //MP4_BOXES_H

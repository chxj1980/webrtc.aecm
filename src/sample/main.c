#include    <stdio.h>
#include    <stdlib.h>
#include    <echo_control_mobile.h>

#define SAMPLE_FREQ 8000

int WebRtcAecTest()
{
#define  NN 160
    short far_frame[NN];
    short near_frame[NN];
    short out_frame[NN];
    void *aecmInst = NULL;
    FILE *fp_far  = fopen("../media/speaker.pcm", "rb");
    FILE *fp_near = fopen("../media/micin.pcm", "rb");
    FILE *fp_out  = fopen("out.pcm", "wb");
    AecmConfig config;

    if(!fp_far || !fp_near || !fp_out) {
        printf("WebRtcAecTest open file err \n");
        return -1;
    }

    WebRtcAecm_Create(&aecmInst);
    WebRtcAecm_Init(aecmInst, SAMPLE_FREQ);

    config.cngMode = 0;
    config.echoMode = 4;
    WebRtcAecm_set_config(aecmInst, config);
    WebRtcAecm_get_config(aecmInst, &config);
    printf("cngMode:%d\n", config.cngMode);
    printf("echoMOde:%d\n", config.echoMode);

    while(1) {
        if (NN == fread(far_frame, sizeof(short), NN, fp_far)) {
            fread(near_frame, sizeof(short), NN, fp_near);
            WebRtcAecm_BufferFarend(aecmInst, far_frame, NN);//对参考声音(回声)的处理
            WebRtcAecm_Process(aecmInst, near_frame, NULL, out_frame, NN, 60);//回声消除
            fwrite(out_frame, sizeof(short), NN, fp_out);
        } else {
            break;
        }
    }

    fclose(fp_far);
    fclose(fp_near);
    fclose(fp_out);
    WebRtcAecm_Free(aecmInst);
    return 0;
}

int main(void)
{
    WebRtcAecTest();
    return 0;
}


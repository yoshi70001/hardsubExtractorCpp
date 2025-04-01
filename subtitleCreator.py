from paddleocr import PaddleOCR
import os
import srt
ocr = PaddleOCR(lang='es',use_angle_cls=False,show_log = False,use_gpu=True)
images =  os.scandir('./_Kashikoi__Ookami_to_Koushinryou___Merchant_Meets_the_Wise_Wolf___01')
subtitles = []
line = 1
for image in images:
    startTime = image.name.split('__')[0]
    startTime=startTime.split('_')
    endTime = image.name.replace('.jpeg','').split('__',13)[1]
    endTime=endTime.split('_')
    print(startTime)
    print(endTime)
    text =''
    result = ocr.ocr(image.path,det=True,rec=True,cls=False)
    try:
        text = result[0][0][1][0]
    except:
        text=''
    subtitles.append(srt.Subtitle(index=line,start=srt.srt_timestamp_to_timedelta(f'{startTime[0]}:{startTime[1]}:{startTime[2]},{startTime[3]}'),end=srt.srt_timestamp_to_timedelta(f'{endTime[0]}:{endTime[1]}:{endTime[2]},{endTime[3]}'),content=text))
print(srt.compose(subtitles))


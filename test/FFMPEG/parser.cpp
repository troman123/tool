int main(int argc, char const *argv[])
{
  char *url = argv[1];
  AVFormatContext *formatContext;
  int trackCount = 0;
  int ret = -1;
  if ((ret = avformat_open_input(&formatContext, url, NULL, NULL)) == 0)
  {
    if (avformat_find_stream_info(formatContext, NULL) >= 0)
    {
      trackCount = formatContext->nb_streams;
    }
  }

  return 0;
}
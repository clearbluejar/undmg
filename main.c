#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "dmg.h"
#include "hfslib.h"
#include "hfsplus.h"

char endianness;

void TestByteOrder() {
  short int word = 0x0001;
  char *byte = (char *)&word;
  endianness = byte[0] ? IS_LITTLE_ENDIAN : IS_BIG_ENDIAN;
}

int main(int argc, char *argv[]) {
  TestByteOrder();

  AbstractFile *in;
  FILE* f;
  if (argc == 2) {
    struct stat status;
    if (stat(argv[1], &status) != 0) {
      perror("stat");
      goto err;
    }
    if (S_ISDIR(status.st_mode)) {
      fprintf(stderr, "error: %s is a directory\n", argv[1]);
      goto err;
    }
    f = fopen(argv[1], "rb");
    in = createAbstractFileFromFile(f);
  } else if (argc > 2) {
    fprintf(stderr, "error: too many arguments provided\n");
    goto err;
  } else {
    if (!isatty(fileno(stdin))) {
      in = createAbstractFileFromFile(stdin);
    } else {
      fprintf(stderr, "error: stdin is a tty, quiting\n");
      goto err;
    }
  }

  AbstractFile *out = createAbstractFileFromFile(tmpfile());

  if (!out) {
    fprintf(stderr, "error: can't create tmp file\n");
    goto err;
  }

  int result = extractDmg(in, out, -1);
  if (!result) {
    fprintf(stderr, "error: the provided data was not a DMG file.\n");
    goto err;
  }

  Volume *volume = openVolume(IOFuncFromAbstractFile(out));

  if (volume == NULL) {
    fprintf(stderr, "error: could not get volume\n");
    goto err;
  }

  HFSPlusCatalogRecord *record = getRecordFromPath("/", volume, NULL, NULL);

  if (record == NULL) {
    fprintf(stderr, "error: could not get HFS record\n");
    closeVolume(volume);
    goto err;
  }

  extractAllInFolder(((HFSPlusCatalogFolder *)record)->folderID, volume);

  fclose(f);
  free(out);
  closeVolume(volume);

  return 0;

 err:
  fclose(f);
  free(out);

  return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#define BUF_LEN 256

// �������x����
static struct timespec tpBefore, tpAfter;
struct timespec GetTime()
{
	struct timespec tp = {0, 0};
	if (clock_gettime(CLOCK_REALTIME, &tp)) {
		printf("clock_gettime error!\n");
	} else {
		//printf("sec=%ld, nsec=%ld\n", tp.tv_sec, tp.tv_nsec);
	}
	return tp;
}
long DiffTime()
{
	long sec = tpAfter.tv_sec - tpBefore.tv_sec;
	long nsec = tpAfter.tv_nsec - tpBefore.tv_nsec;
	if (nsec < 0) {
		sec--;
		nsec += 1000000000L;
	}
	return (sec * 1000000000L + nsec);
}

// 1�o�C�g�Â��[�v�R�s�[
void *test1(void *dest, const void *src, size_t n)
{
	const int ADD = 1;
	char *rbuf = (char*)src;
	char *wbuf = (char*)dest;
	for (int i=0; i<n; i+=ADD) {
		memcpy(wbuf, rbuf, ADD);
		rbuf += ADD;
		wbuf += ADD;
	}
	return dest;
}

// 8�o�C�g�Â��[�v�R�s�[
void *test2(void *dest, const void *src, size_t n)
{
	const int ADD = 8;
	char *rbuf = (char*)src;
	char *wbuf = (char*)dest;
	for (int i=0; i<n; i+=ADD) {
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
		memcpy(wbuf, rbuf, 1); rbuf += 1; wbuf += 1;
	}
	return dest;
}

// �}���`�X���b�h�ŕ����R�s�[
typedef struct {
	void *dest;
	const void *src;
	size_t n;
} cpybuf_t;
void *test3_sub(void *arg)
{
	cpybuf_t *buf = (cpybuf_t*)arg;
	//test1(buf->dest, buf->src, buf->n);
	memcpy(buf->dest, buf->src, buf->n);
	return 0;
}
void *test3(void *dest, const void *src, size_t n)
{
	const int THSIZE = 3;
	pthread_t pt[THSIZE];
	cpybuf_t buf[THSIZE];
	
	buf[0].dest = dest;
	buf[0].src = src;
	buf[0].n = n / THSIZE;
	pthread_create(&pt[0], NULL, &test3_sub, &buf[0]);

	for (int i=1; i<THSIZE; i++) {
		buf[i].dest = (char*)buf[i-1].dest + buf[0].n;
		buf[i].src = (char*)buf[i-1].src + buf[0].n;
		buf[i].n = buf[0].n;
		pthread_create(&pt[i], NULL, &test3_sub, &buf[i]);
	}

	for (int i=0; i<THSIZE; i++) { 
		pthread_join(pt[i], NULL);
	}
	return dest;
}

// ���C��
int main(int argc, char **argv)
{
	FILE *fp = NULL;
	char buf[BUF_LEN];
	char *fbuf = NULL;
	char *wbuf = NULL;
	char *bk = NULL;
	int err = 0;
	int rlen = 0;
	int size = 0;
	
	// �����`�F�b�N
	if (argc < 3) {
		return 1;
	}

	// �Ǎ��t�@�C���I�[�v��
	if ((fp = fopen(argv[1], "rb")) == 0) {
		printf("read file open error: %s", argv[1]);
		return 1;
	}
	
	// �Ǎ��t�@�C���T�C�Y�擾
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("read file size=%d\n", size);
	
	// �������m��
	fbuf = (char*)malloc(size);
	bk = fbuf;	// �Ǎ��������̐擪�|�C���^��ޔ�
	wbuf = (char*)malloc(size);
	
	// �t�@�C���Ǎ�
	while((rlen = fread(buf, sizeof(char), BUF_LEN, fp)) == BUF_LEN) {
		memcpy(fbuf, buf, rlen);
		fbuf += rlen;
	}
	if (rlen > 0) {
		memcpy(fbuf, buf, rlen);
	}
	fclose(fp);

	fbuf = bk;	// �Ǎ��������̃|�C���^��擪�ɖ߂�
	
	// �f�[�^�R�s�[	
	tpBefore = GetTime();
	test1(wbuf, fbuf, size);
	tpAfter = GetTime();
	printf("test01: %lfsec\n", (double)DiffTime() / 1000000000);

	tpBefore = GetTime();
	test2(wbuf, fbuf, size);
	tpAfter = GetTime();
	printf("test02: %lfsec\n", (double)DiffTime() / 1000000000);

	tpBefore = GetTime();
	test3(wbuf, fbuf, size);
	tpAfter = GetTime();
	printf("test03: %lfsec\n", (double)DiffTime() / 1000000000);

	tpBefore = GetTime();
	memcpy(wbuf, fbuf, size);
	tpAfter = GetTime();
	printf("memcpy: %lfsec\n", (double)DiffTime() / 1000000000);
		
	// �����t�@�C���I�[�v��
	if ((fp = fopen(argv[2], "wb")) == 0) {
		printf("write file open error: %s", argv[2]);
		return 1;
	}
	
	// �t�@�C������
	fwrite(wbuf, sizeof(char), size, fp);
	fclose(fp);
	
	// �������J��
	free(fbuf);
	free(wbuf);
	
	return 0;
}

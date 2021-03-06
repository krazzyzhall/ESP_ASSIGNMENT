#define ISPATTERN      101
#define SEQ_ENABLE     103
#define KILL_THREAD    104
#define INS_SEQ        109

#define SPI_LSB_FIRST   0x08
#define SPI_MODE_0      (0|0)
#define SPI_TX_DUAL	0x100
#define SPI_TX_QUAD	0x200
#define SPI_RX_DUAL	0x400
#define SPI_RX_QUAD	0x800
#define SPI_MODE_MASK           (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                                | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                                | SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
                                | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)
#define N_SPI_MINORS                    32      /* ... up to 256 */
#define SPI_MAJOR 153
struct my_seq_pat{
int *seq;
int lseq;
};

struct kthreadp{

struct spidev_data              *spidev;
size_t  			count;
int 				status;
};

/* function for checking err */




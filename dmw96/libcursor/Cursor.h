/**************************************************/
/* dmw96 lcdc cursor                              */
/**************************************************/

class Cursor
{
public:
    Cursor(const char *devName);
    ~Cursor();

    int setDefaultPallete();
    int setDefaultBitmap();
    int setAlternativeBitmap();
    int setPosition(int x, int y);
    int setVisible(bool visible);

    bool isInitialized;
private:
    int setBitmap(unsigned char *cursor, unsigned int size);

    int fd;

    bool isVisible;
};



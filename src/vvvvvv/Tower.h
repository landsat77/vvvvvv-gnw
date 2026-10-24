#ifndef TOWER_H
#define TOWER_H

class towerclass
{
public:
    towerclass(void);

    int backat(int xp, int yp, int yoff);

    int at(int xp, int yp, int yoff);

    int miniat(int xp, int yp, int yoff);

    void loadminitower1(void);

    void loadminitower2(void);

    void loadbackground(void);

    void loadmap(void);

#ifdef GNW
    /* the tables of the pack (vv_levels.bin), read in place; loaded at
     * first use (the pack is not open yet when the constructor runs) */
    const short* back;
    const short* contents;
    const short* minitower;
#else
    short back[40 * 120];
    short contents[40 * 700];
    short minitower[40 * 100];
#endif

    bool minitowermode;
};





#endif /* TOWER_H */

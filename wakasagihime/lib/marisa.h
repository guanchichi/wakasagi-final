// Chinese Dark Chess: Magic
// ----------------------------------
#ifndef MARISA_DA_ZE
#define MARISA_DA_ZE
/*_______________________________
( Ordinary magician's bitboards! )
 --------------------------------
             \
              \                        _-%%==-_
               \                    %@*        #@@=
                \                @@*               *@.
                 \            .@*                    *@+,
                  \         #@                          *@
                          @@   =@%*#@@**,      =@@**%@@=  #@
                        @%   .@          @@*@*         *%-__@.
                      @#     %.      --==*@  @*==-       #*
                    @#        @        __##@@@##__,     @  @
                  %@  __+%@@@@@@%%#@#=*^          ^**=:+@##*==__,
               ..@@@**                                          *@@:.
           ,=@#*            ___=+%+.   ___--=+.   __-=+-,    *#.    *@#
       .+@**   .@  =@:___%@#      .@##:        @%:       =,    @       @.
     .@+       #_=*             @                          *=@*         @
    .@           @      @       %.       *@       +          @=        .@
    .@          :@      #     ,= @      .@ @       #        #. %     ,@#
     .@.         @      .@.%@%    @,    @   **==__# @      #=  @__=@@*
       ^*:*###@@@@.     @          *@=__@:           %@:=@#   @.
                 ,@   -# ===========     ===========  @    ^*=
                #  *_  @   @  @    @     @  @    @   #       #
              .@  .@ *#=-* @*^     @     #*^     @   #       @
             #@-%* @   @    @     @.      @     ,*    @     @
                 @  #  @     *@@@+         *@@@*       @+*%@
                 @  ^#  @                             @     @
                 @   @^--*                           %.     .*
                 *. .# @.           ^*_*^             @     @
                  ^*-#  *#=_                      _,@@%*   *#%@_
                           ^:@=___         ___==#@ @    @*@    @
                              *   ^*-----*^ @    : %  _:@*@:_  @
                             @__  #         # __=@  ^* @  @  *^
                                ^*=--++++--**^        *    #.
                                                      @  * @
                                                      % @ ^#
                                                       ^*/

#include "chess.h"
#include <immintrin.h>

/*
 * Parallel Bit Extract
 * @internal
 */
unsigned pext(unsigned b, unsigned m);

// -~ Magic bitboards ~-
// Calculate moves for sliding pieces (chariots, cannons) really fast
// (It's really just a hash table)
struct Magic {
    Board mask;
    Board *attacks;
    // Magic index
    unsigned index(Board occupied) const { return pext(occupied, mask); }
    Board attacks_bb(Board occupied) const { return attacks[index(occupied)]; }
};

extern Board cannonTable[];
extern Magic cannonMagics[SQUARE_NB];

/*
 * @internal
 * @param   s       The origin square
 * @param   step    A Direction to move from the origin
 * @returns The bitboard of the square reached by taking _step_ from _s_ if move is legal
 *          The empty bitboard if the move is not legal
 */
Board safe_destination(Square s, int step);

/*
 * The girls are preparing...
 * @internal
 */
template<PieceType pt>
void init_magic(Board table[], Magic magics[]);

#endif
# nbtcmd
edit nbt in game
# Usage
## command format
  #### nbt from to
  #### from,to:
  ##### 1.string:read/write file(snbt)
  ##### 2.CommandSelector<Actor>:read/write nbt from/to actor(cannot write nbt from actors)
  ##### 3.CommandPosition:read/write nbt from/to block(cannot write nbt from actors)
  ##### 4.solt CommandSelector<Player>:read/write nbt from/to item(cannot write nbt from actors)
  #### setdefaultoutputmode mode
  ##### mode
  ###### 1.book(out to a book)
  ###### 2.consle(out nbt to consle)
  ###### 3.commandOutPut(command output)

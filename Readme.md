# How to run
1. Build ns-3 [Guide](https://www.nsnam.org/docs/release/3.41/installation/ns-3-installation.pdf) - Надо ставить все required и optional, чтобы потом вопросов не было
2. Clone this repo in the folder, where ns3 binary placed (named **ns3-folder** below)
3. Copy main.cc file into scratch folder. In ns3-folder:
```bash
cp ${REPO}/main.cc scratch/
```
4. Run program:
```bash
./ns3 run scratch/main.cc
```

## Attention
- Я не знаю, как запустить, если главный файл разделить на несколько!
Надо какой-то модуль создать, тяжело с этим разобраться
- Код пока что полностью не рабочий: ноды, девайсы создаются, а вот пакеты не передаются
- Надо как-то еще и импортнуть реализованный бэтмен, похоже надо все таки создавать модуль...
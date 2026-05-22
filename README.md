# SimpleFS

## Запуск

Сборка
```bash
make
```

Создать виртуальный диск
```bash
sudo dd if=/dev/zero of=disk.img bs=512 count=2050
```

Подключить диск к loop device

```bash
export DEVICE=$(sudo losetup -f --show disk.img)
echo "$DEVICE"
```

Загрузить модуль ядра

```bash
sudo insmod simplefs.ko device_name="$DEVICE" \
    sb_main_sector=0 \
    sb_backup_sector=1024 \
    max_name_len=64 \
    max_file_sectors=16
```

Смонтировать файловую систему

```bash
sudo mkdir -p /mnt/simplefs
sudo mount -t simplefs $DEVICE /mnt/simplefs
```

Просмотреть список файлов

```bash
ls -la /mnt/simplefs
```

Использование утилиты CLI
Проверка чтения/записи
```bash
./simplefs_cli /mnt/simplefs test
```

Получение метаданных о файлах
```bash
./simplefs_cli /mnt/simplefs metadata
```

Получения информации о секторах файла
```bash
./simplefs_cli /mnt/simplefs <filename>
```

Заполнения файловых секторов нулями
```bash
./simplefs_cli /mnt/simplefs zero
```

Полная очистка файловой системы
```bash
./simplefs_cli /mnt/simplefs erase
```

## Тестовый скрипт
```bash
sudo ./scripts/test.sh
```
Скрипт делает следующие действия:
1. Собирает модуль и утилиту
2. Создаёт виртуальный образ диска и подключает его как loop device
3. Загружает модуль ядра
4. Монтирует файловую систему
5. Использует различные опции пользовательской утилиты

В конце очищает созданные ресурсы

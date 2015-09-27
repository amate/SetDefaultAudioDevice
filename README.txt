/* ==============================
       SetDefaultAudioDevice
 ===============================*/
 
 windows vista / 7 / 8 / 10 でデフォルトのオーディオデバイスを変更します

■使い方
起動するとポップアップメニューが開かれるので、規定のオーディオデバイスに指定したいデバイスを選択してください。

また、コマンドライン引数でトグル動作させることができます。
SetDefaultAudioDevice.exe -t"トグル動作させたいデバイス名1" -t"トグル動作させたいデバイス名2"
例: SetDefaultAudioDevice.exe -t"スピーカー (Realtek AC'97 Audio)" -t"スピーカー (C-Media USB Audio       )"

デバイス名はSetDefaultAudioDeviceをCtrlを押しながら起動することによって、全デバイス名がクリップボードにコピーされるので参考にしてください

コマンドライン引数で指定するデバイス名は部分一致で検索するので、デバイス名の一部を指定するだけでもかまいません
上の例だと
SetDefaultAudioDevice.exe -t"Realtek" -t"C-Media"
でも動作します

■免責
このソフトウェアを使用して起きたいかなる損害について作者は責任を負わないものとします。
また、このソフトウェアの不具合修正について作者はいかなる義務も負わないものとします。

■参考
このアプリケーションは下のURLで配布されているソースを元に作成しました
https://github.com/xenolightning/AudioSwitcher

License : Microsoft Public License (Ms-PL)
 
■更新履歴

ver 1.2  windows10で動作しなかったのを修正

ver 1.1  トグル動作を追加

ver 1.0  公開
 
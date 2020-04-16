<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja_JP">
<context>
    <name>Exporter</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="576"/>
        <source>Could not write file: %1</source>
        <translation>ファイルを書き込みできません: %1</translation>
    </message>
</context>
<context>
    <name>GdalImageReader</name>
    <message>
        <location filename="../src/gdal/gdal_image_reader.cpp" line="56"/>
        <location filename="../src/gdal/gdal_image_reader.cpp" line="143"/>
        <source>Failed to read image data: %1</source>
        <translation type="unfinished">画像データの読み取りに失敗しました：％1</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_image_reader.cpp" line="112"/>
        <source>Unsupported raster data: %1</source>
        <translation type="unfinished">サポートされていないラスターデータ：％1</translation>
    </message>
</context>
<context>
    <name>Importer</name>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="648"/>
        <source>Cannot open file
%1:
%2</source>
        <translation>ファイルを開くことができません
%1:
%2</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="648"/>
        <source>Cannot open file:
%1

%2</source>
        <translation>ファイルを開くことができません:
%1

%2</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="649"/>
        <source>Invalid file type.</source>
        <translation>無効なファイル形式です。</translation>
    </message>
</context>
<context>
    <name>Map</name>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="938"/>
        <source>Question</source>
        <translation>質問</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="939"/>
        <source>The scale of the imported data is 1:%1 which is different from this map&apos;s scale of 1:%2.

Rescale the imported data?</source>
        <translation>インポートされた地図の縮尺 1:%1 と、編集中の地図の縮尺 1:%2 が異なります。

インポートされた地図の縮尺を変更しますか?</translation>
    </message>
</context>
<context>
    <name>OcdFileExport</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="577"/>
        <source>OCD files of version %1 are not supported!</source>
        <translation>バージョン %1 の OCD ファイルはサポートされていません!</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="662"/>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="673"/>
        <source>Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m).</source>
        <translation>座標は、OCAD 8 描画領域に収まるように調整されます (-2 m ... 2 m)。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="683"/>
        <source>Some coordinates remain outside of the OCAD 8 drawing area. They might be unreachable in OCAD.</source>
        <translation>一部の座標が OCAD 8 描画領域外に残っています。 OCAD で到達できない可能性があります。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="717"/>
        <source>The georeferencing cannot be saved in OCD version 8.</source>
        <translation>ジオリファレンスは OCD バージョン 8 で保存できません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="767"/>
        <source>The map contains more than 24 spot colors which is not supported by OCD version 8.</source>
        <translation>マップに、OCD バージョン 8 ではサポートされていない 24 色以上のスポットカラーが含まれています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="773"/>
        <source>The map contains more than 256 colors which is not supported by OCD version 8.</source>
        <translation>マップに、OCD バージョン 8 ではサポートされていない 256 色以上の色が含まれています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="817"/>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="829"/>
        <source>Invalid spot color.</source>
        <translation>スポットカラーが無効です。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="2462"/>
        <source>Unable to save correct position of missing template: &quot;%1&quot;</source>
        <translation>見つからないテンプレートの正しい位置を保存できません: &quot;%1&quot;</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="2478"/>
        <source>Unable to export template: file type of &quot;%1&quot; is not supported yet</source>
        <translation>テンプレートをエクスポートできません: ファイル形式 &quot;%1&quot; はまだサポートされていません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="2727"/>
        <source>Text truncated at &apos;|&apos;): %1</source>
        <translation>テキストが &apos;|&apos;) で切り捨てられました: %1</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering</name>
    <message>
        <location filename="../packaging/translations.cpp" line="10"/>
        <source>Orienteering map</source>
        <translation>オリエンテーリング地図</translation>
    </message>
    <message>
        <location filename="../packaging/translations.cpp" line="11"/>
        <source>Software for drawing orienteering maps</source>
        <translation>オリエンテーリングの地図を描画するためのソフトウェア</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="548"/>
        <source>Encoding &apos;%1&apos; is not available. Check the settings.</source>
        <translation>エンコード &apos;%1&apos; は使用できません。設定を確認してください。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="2061"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="2107"/>
        <source>Failed to create feature in layer: %1</source>
        <translation>レイヤーにフィーチャーを作成できませんでした: %1</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="2117"/>
        <source>Failed to create layer %1: %2</source>
        <translation>レイヤー %1 の作成に失敗しました: %2</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="2124"/>
        <source>Failed to create name field: %1</source>
        <translation>名前フィールドの作成に失敗しました: %1</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::AboutDialog</name>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="105"/>
        <location filename="../src/gui/about_dialog.cpp" line="175"/>
        <source>About %1</source>
        <translation>%1 について</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="180"/>
        <source>This program is free software: you can redistribute it and/or modify it under the terms of the &lt;a %1&gt;GNU General Public License (GPL), version&amp;nbsp;3&lt;/a&gt;, as published by the Free Software Foundation.</source>
        <translation>このプログラムはフリー ソフトウェアです: フリー ソフトウェア財団によって公開される &lt;a %1&gt;GNU 一般公衆利用許諾書 (GPL)、バージョン&amp;nbsp;3&lt;/a&gt; の条件の下で再配布および/または変更することができます。</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="185"/>
        <source>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License (GPL), version&amp;nbsp;3, for &lt;a %1&gt;more details&lt;/a&gt;.</source>
        <translation>このプログラムは有用であることを願って頒布されますが、全くの無保証です。商業可能性の保証や特定目的への適合性は、言外に示されたものも含め、全く存在しません。&lt;a %1&gt;詳細&lt;/a&gt;は GNU 一般公衆利用許諾書 (GPL)、バージョン&amp;nbsp;3 をご覧くださ い。</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="190"/>
        <source>&lt;a %1&gt;All about licenses, copyright notices, conditions and disclaimers.&lt;/a&gt;</source>
        <translation>&lt;a %1&gt;ライセンス、著作権通知、条件、および免責事項に関するすべて。&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="192"/>
        <source>The OpenOrienteering developers in alphabetical order:</source>
        <translation>アルファベット順の OpenOrienteering 開発者:</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="193"/>
        <source>(project initiator)</source>
        <translation>(プロジェクト開始)</translation>
    </message>
    <message>
        <source>License (%1)</source>
        <translation type="vanished">ライセンス (%1)</translation>
    </message>
    <message>
        <location filename="../packaging/translations.cpp" line="14"/>
        <location filename="../src/gui/about_dialog.cpp" line="178"/>
        <source>A free software for drawing orienteering maps</source>
        <extracomment>For the moment, we use this existing translation instead of the previous one.</extracomment>
        <translation>オリエンテーリング地図作成のための自由なソフトウェア</translation>
    </message>
    <message>
        <source>Developers in alphabetical order:</source>
        <translation type="vanished">デベロッパ (アルファベット順):</translation>
    </message>
    <message>
        <location filename="../src/gui/about_dialog.cpp" line="194"/>
        <source>For contributions, thanks to:</source>
        <translation>コントリビュータ:</translation>
    </message>
    <message>
        <source>This program uses the &lt;b&gt;Clipper library&lt;/b&gt; by Angus Johnson.</source>
        <translation type="vanished">このプログラムは &lt;b&gt;Clipper library&lt;/b&gt; by Angus Johnson を使用しています。</translation>
    </message>
    <message>
        <source>See &lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt; for more information.</source>
        <translation type="vanished">詳細については、&lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt; を参照してください。</translation>
    </message>
    <message>
        <source>Additional information</source>
        <translation type="vanished">追加情報</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::AbstractHomeScreenWidget</name>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="116"/>
        <source>Open most recently used file</source>
        <translation>直前に使用したファイルを開く</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="119"/>
        <source>Show tip of the day</source>
        <translation>今日のヒントを表示する</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ActionGridBar</name>
    <message>
        <location filename="../src/gui/widgets/action_grid_bar.cpp" line="59"/>
        <source>Show remaining items</source>
        <translation>残りのアイテムを表示</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::AreaSymbolSettings</name>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="86"/>
        <source>mm²</source>
        <translation>mm²</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="87"/>
        <source>Minimum size:</source>
        <translation>最小のサイズ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="94"/>
        <source>Fills</source>
        <translation>フィル</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="107"/>
        <source>Line fill</source>
        <translation>ラインフィル</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="108"/>
        <source>Pattern fill</source>
        <translation>パターンフィル</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="146"/>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="157"/>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="164"/>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="183"/>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="188"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="161"/>
        <source>Single line</source>
        <translation>シングルライン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="175"/>
        <source>Parallel lines</source>
        <translation>平行線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="177"/>
        <source>Line spacing:</source>
        <translation>平行線の間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="180"/>
        <source>Single row</source>
        <translation>シングルロウ</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="182"/>
        <source>Pattern interval:</source>
        <translation>パターン間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="187"/>
        <source>Pattern offset:</source>
        <translation>パターンのオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="192"/>
        <source>Row offset:</source>
        <translation>ロウのオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="194"/>
        <source>Parallel rows</source>
        <translation>平行ロウ</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="196"/>
        <source>Row spacing:</source>
        <translation>ロウ間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="202"/>
        <source>Fill rotation</source>
        <translation>フィルの回転</translation>
    </message>
    <message>
        <source>°</source>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="207"/>
        <source>Angle:</source>
        <translation>角度:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="209"/>
        <source>adjustable per object</source>
        <translation>オブジェクトごとに調節可能</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="229"/>
        <source>Element drawing at boundary</source>
        <translation>境界で描画する要素</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="231"/>
        <source>Clip elements at the boundary.</source>
        <translation>要素を境界で切り抜きます。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="232"/>
        <source>Draw elements if the center is inside the boundary.</source>
        <translation>中心が境界の内部にある場合に、要素を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="233"/>
        <source>Draw elements if any point is inside the boundary.</source>
        <translation>任意のポイントが境界内にある場合に、要素を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="234"/>
        <source>Draw elements if all points are inside the boundary.</source>
        <translation>すべてのポイントが境界内にある場合に、要素を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="249"/>
        <source>Area settings</source>
        <translation>エリアの設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="343"/>
        <source>Pattern fill %1</source>
        <translation>パターンフィル %1</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="352"/>
        <source>Line fill %1</source>
        <translation>ラインフィル %1</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="371"/>
        <source>No fill selected</source>
        <translation>フィルが選択されていません</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="84"/>
        <source>Area color:</source>
        <translation>エリアの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="173"/>
        <source>Line offset:</source>
        <translation>ラインのオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="168"/>
        <source>Line color:</source>
        <translation>ラインの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/area_symbol_settings.cpp" line="163"/>
        <source>Line width:</source>
        <translation>ラインの幅:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::AutosaveDialog</name>
    <message>
        <location filename="../src/gui/autosave_dialog.cpp" line="46"/>
        <source>Autosaved file</source>
        <translation>自動保存したファイル</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/autosave_dialog.cpp" line="48"/>
        <location filename="../src/gui/autosave_dialog.cpp" line="54"/>
        <source>%n bytes</source>
        <translation>
            <numerusform>%n バイト</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/gui/autosave_dialog.cpp" line="52"/>
        <source>File saved by the user</source>
        <translation>ユーザーによって保存されたファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/autosave_dialog.cpp" line="59"/>
        <source>File recovery</source>
        <translation>ファイルの復元</translation>
    </message>
    <message>
        <location filename="../src/gui/autosave_dialog.cpp" line="61"/>
        <source>File %1 was not properly closed. At the moment, there are two versions:</source>
        <translation>ファイル %1 は正しく閉じられませんでした。現時点では、2 つのバージョンがあります:</translation>
    </message>
    <message>
        <location filename="../src/gui/autosave_dialog.cpp" line="75"/>
        <source>Save the active file to remove the conflicting version.</source>
        <translation>作業中のファイルを保存して、競合するバージョンを削除します。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ColorDialog</name>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="86"/>
        <source>Edit map color</source>
        <translation>色の編集</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="93"/>
        <source>Edit</source>
        <translation>編集</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="101"/>
        <source>Spot color printing</source>
        <translation>スポットカラー印刷</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="106"/>
        <source>Defines a spot color:</source>
        <translation>スポットカラーの定義:</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="115"/>
        <source>Screen frequency:</source>
        <translation>画面周波数:</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="116"/>
        <source>lpi</source>
        <translation>lpi</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="117"/>
        <source>Undefined</source>
        <translation>未定義</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="121"/>
        <source>Screen angle:</source>
        <translation>画面の角度:</translation>
    </message>
    <message>
        <source>°</source>
        <translation type="obsolete">°</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="128"/>
        <source>Mixture of spot colors (screens and overprint):</source>
        <translation>スポットカラーの混合(スクリーンとオーバープリント):</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="143"/>
        <location filename="../src/gui/color_dialog.cpp" line="181"/>
        <location filename="../src/gui/color_dialog.cpp" line="186"/>
        <location filename="../src/gui/color_dialog.cpp" line="191"/>
        <location filename="../src/gui/color_dialog.cpp" line="196"/>
        <location filename="../src/gui/color_dialog.cpp" line="440"/>
        <source>%</source>
        <translation>%</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="148"/>
        <source>Knockout: erases lower colors</source>
        <translation>ノックアウト(抜き): 下の色を消去</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="161"/>
        <source>CMYK</source>
        <translation>CMYK</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="166"/>
        <location filename="../src/gui/color_dialog.cpp" line="223"/>
        <source>Calculate from spot colors</source>
        <translation>スポットカラーから計算</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="171"/>
        <source>Calculate from RGB color</source>
        <translation>RGBカラーから計算</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="176"/>
        <source>Custom process color:</source>
        <translation>カスタム・プロセスカラー:</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="182"/>
        <source>Cyan</source>
        <translation>シアン</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="187"/>
        <source>Magenta</source>
        <translation>マゼンダ</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="192"/>
        <source>Yellow</source>
        <translation>イエロー</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="197"/>
        <source>Black</source>
        <translation>ブラック</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="218"/>
        <source>RGB</source>
        <translation>RGB</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="228"/>
        <source>Calculate from CMYK color</source>
        <translation>CMYKカラーから計算</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="233"/>
        <source>Custom RGB color:</source>
        <translation>カスタム・RGBカラー:</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="239"/>
        <source>Red</source>
        <translation>赤</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="244"/>
        <source>Green</source>
        <translation>緑</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="249"/>
        <source>Blue</source>
        <translation>青</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="254"/>
        <source>#RRGGBB</source>
        <translation>#RRGGBB</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="272"/>
        <source>Desktop</source>
        <translation>デスクトップ</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="273"/>
        <source>Professional printing</source>
        <translation>プロフェッショナル印刷</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="292"/>
        <source>Name</source>
        <translation>名前</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="346"/>
        <source>- unnamed -</source>
        <translation>- 名称未設定 -</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="623"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ColorDropDown</name>
    <message>
        <location filename="../src/gui/widgets/color_dropdown.cpp" line="42"/>
        <source>- none -</source>
        <translation>- none -</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ColorListWidget</name>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>Name</source>
        <translation>名前</translation>
    </message>
    <message>
        <source>C</source>
        <translation type="obsolete">C</translation>
    </message>
    <message>
        <source>M</source>
        <translation type="obsolete">M</translation>
    </message>
    <message>
        <source>Y</source>
        <translation type="obsolete">Y</translation>
    </message>
    <message>
        <source>K</source>
        <translation type="obsolete">K</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>Opacity</source>
        <translation>不透明度</translation>
    </message>
    <message>
        <source>R</source>
        <translation type="obsolete">R</translation>
    </message>
    <message>
        <source>G</source>
        <translation type="obsolete">G</translation>
    </message>
    <message>
        <source>B</source>
        <translation type="obsolete">B</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>Spot color</source>
        <translation>スポットカラー</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>CMYK</source>
        <translation>CMYK</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>RGB</source>
        <translation>RGB</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="83"/>
        <source>K.o.</source>
        <translation>K.o.</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="89"/>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="94"/>
        <source>New</source>
        <translation>新規</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="97"/>
        <source>Delete</source>
        <translation>削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="90"/>
        <source>Duplicate</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="103"/>
        <source>Move Up</source>
        <translation>上へ移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="105"/>
        <source>Move Down</source>
        <translation>下へ移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="116"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="137"/>
        <source>Double-click a color value to open a dialog.</source>
        <translation>色をダブルクリックするとダイアログが開きます。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="232"/>
        <source>Confirmation</source>
        <translation>確認</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="232"/>
        <source>The map contains symbols with this color. Deleting it will remove the color from these objects! Do you really want to do that?</source>
        <translation>地図にこの色の記号が含まれています。削除するとそれらのオブジェクトからも色が削除されます。本当に削除しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="250"/>
        <source>%1 (duplicate)</source>
        <extracomment>Future replacement for COLOR_NAME + &quot; (Duplicate)&quot;, for better localization.</extracomment>
        <translation>%1 (複製)</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="251"/>
        <source> (Duplicate)</source>
        <translation> (複製)</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="453"/>
        <source>%1 (%2°, %3 lpi)</source>
        <translation>%1 (%2°, %3 lpi)</translation>
    </message>
    <message>
        <source>Error</source>
        <translation type="vanished">エラー</translation>
    </message>
    <message>
        <source>Please enter a percentage from 0% to 100%!</source>
        <translation type="vanished">0%から100%の値を入力してください!</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="409"/>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="466"/>
        <source>Double click to define the color</source>
        <translation>ダブルクリックで色を定義します</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="417"/>
        <source>Click to select the name and click again to edit.</source>
        <translation>名前をクリックで選択。もう一度クリックで編集。</translation>
    </message>
    <message>
        <source>Please enter a valid number from 0 to 255, or specify a percentage from 0 to 100!</source>
        <translation type="obsolete">0から255の有効な数字を入力してください。または0から100のパーセントで指定してください!</translation>
    </message>
    <message>
        <source>Double click to pick a color</source>
        <translation type="obsolete">ダブルクリックで色を選択します</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::CombinedSymbolSettings</name>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="87"/>
        <source>&amp;Number of parts:</source>
        <translation>要素の数(&amp;N):</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="95"/>
        <source>- Private line symbol -</source>
        <translation>- プライベート・ライン記号-</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="96"/>
        <source>- Private area symbol -</source>
        <translation>- プライベート・エリア記号-</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="99"/>
        <source>Edit private symbol...</source>
        <translation>プライベート記号の編集...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="113"/>
        <source>Combination settings</source>
        <translation>組み合わせの設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="226"/>
        <source>Change from public to private symbol</source>
        <translation>従来の記号からプライベート記号を作成</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="227"/>
        <source>Take the old symbol as template for the private symbol?</source>
        <translation>選択中の記号からプライベート記号を作成しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/combined_symbol_settings.cpp" line="92"/>
        <source>Symbol %1:</source>
        <translation>記号 %1:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ConfigureGridDialog</name>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="67"/>
        <source>Configure grid</source>
        <translation>グリッド設定</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="69"/>
        <source>Show grid</source>
        <translation>グリッドを表示</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="70"/>
        <source>Snap to grid</source>
        <translation>グリッドにスナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="71"/>
        <source>Choose...</source>
        <translation>選択...</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="74"/>
        <source>All lines</source>
        <translation>全ての線</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="75"/>
        <source>Horizontal lines</source>
        <translation>横線</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="76"/>
        <source>Vertical lines</source>
        <translation>縦線</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="128"/>
        <source>Alignment</source>
        <translation>アラインメント</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="78"/>
        <source>Align with magnetic north</source>
        <translation>磁北線に合わせる</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="79"/>
        <source>Align with grid north</source>
        <translation>グリッド北に合わせる</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="80"/>
        <source>Align with true north</source>
        <translation>真北に合わせる</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="82"/>
        <source>Additional rotation (counter-clockwise):</source>
        <translation>追加回転 (反時計回り):</translation>
    </message>
    <message>
        <source>°</source>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="135"/>
        <source>Positioning</source>
        <translation>位置調整</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="88"/>
        <source>meters in terrain</source>
        <translation>テレイン内での距離 (m)</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="89"/>
        <source>millimeters on map</source>
        <translation>地図上での長さ (mm)</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="91"/>
        <source>Horizontal spacing:</source>
        <translation>水平間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="93"/>
        <source>Vertical spacing:</source>
        <translation>垂直間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="97"/>
        <source>Horizontal offset:</source>
        <translation>水平オフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="99"/>
        <source>Vertical offset:</source>
        <translation>垂直オフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="136"/>
        <source>Unit:</source>
        <comment>measurement unit</comment>
        <translation>測定単位:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="124"/>
        <source>Line color:</source>
        <translation>ラインの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="125"/>
        <source>Display:</source>
        <translation>表示:</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="171"/>
        <source>Choose grid line color</source>
        <translation>グリッド線の色を選択</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="255"/>
        <source>m</source>
        <comment>meters</comment>
        <translation>m</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="255"/>
        <source>mm</source>
        <comment>millimeters</comment>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="261"/>
        <source>Origin at: %1</source>
        <translation>原点の位置: %1</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="263"/>
        <source>paper coordinates origin</source>
        <translation>ペーパー座標の原点</translation>
    </message>
    <message>
        <location filename="../src/gui/configure_grid_dialog.cpp" line="265"/>
        <source>projected coordinates origin</source>
        <translation>投影座標の原点</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::CutHoleTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; on a line to split it into two, &lt;b&gt;Drag&lt;/b&gt; along a line to remove this line part, &lt;b&gt;Click or Drag&lt;/b&gt; at an area boundary to start drawing a split line</source>
        <translation type="obsolete">ライン上を&lt;b&gt;クリック&lt;/b&gt;でラインを分割、&lt;b&gt;ドラッグ&lt;/b&gt;でラインの部分を消去、エリアの輪郭上を&lt;b&gt;クリックまたはドラッグ&lt;/b&gt;でエリアの分割ラインを開始します</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_hole_tool.cpp" line="259"/>
        <source>&lt;b&gt;Click or drag&lt;/b&gt;: Start drawing the hole. </source>
        <translation>&lt;b&gt;クリックまたはドラッグ&lt;/b&gt;: 穴の描画を開始します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::CutTool</name>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="373"/>
        <location filename="../src/tools/cut_tool.cpp" line="421"/>
        <location filename="../src/tools/cut_tool.cpp" line="427"/>
        <location filename="../src/tools/cut_tool.cpp" line="433"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="421"/>
        <source>The split line must end on the area boundary!</source>
        <translation>分割ラインはエリアの輪郭上で終わらせてください!</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="427"/>
        <source>Start and end of the split line are at different parts of the object!</source>
        <translation>分割ラインの始点と終点がオブジェクトの異なる部分にあります!</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="433"/>
        <source>Start and end of the split line are at the same position!</source>
        <translation>分割ラインの始点と終点が同じ場所にあります!</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="108"/>
        <source>&lt;b&gt;Click&lt;/b&gt; on a line: Split it into two. &lt;b&gt;Drag&lt;/b&gt; along a line: Remove this line part. &lt;b&gt;Click or Drag&lt;/b&gt; at an area boundary: Start a split line. </source>
        <translation>ライン上を&lt;b&gt;クリック&lt;/b&gt;: ラインを分割。ライン上を&lt;b&gt;ドラッグ&lt;/b&gt;: ラインの一部を消去。エリアの輪郭を&lt;b&gt;クリックまたはドラッグ&lt;/b&gt;: エリアの分割を開始。 </translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; on a line to split it into two, &lt;b&gt;Drag&lt;/b&gt; along a line to remove this line part, &lt;b&gt;Click or Drag&lt;/b&gt; at an area boundary to start drawing a split line</source>
        <translation type="obsolete">ライン上を&lt;b&gt;クリック&lt;/b&gt;でラインを分割、&lt;b&gt;ドラッグ&lt;/b&gt;でラインの部分を消去、エリアの輪郭上を&lt;b&gt;クリックまたはドラッグ&lt;/b&gt;でエリアの分割ラインを開始します</translation>
    </message>
    <message>
        <location filename="../src/tools/cut_tool.cpp" line="373"/>
        <source>Splitting holes of area objects is not supported yet!</source>
        <translation>エリアオブジェクトの穴の分割はまだサポートされていません!</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::CutoutTool</name>
    <message>
        <location filename="../src/tools/cutout_tool.cpp" line="148"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Clip the whole map. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 地図全体をクリップします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/cutout_tool.cpp" line="149"/>
        <location filename="../src/tools/cutout_tool.cpp" line="153"/>
        <source>&lt;b&gt;%1+Click or drag&lt;/b&gt;: Select the objects to be clipped. </source>
        <translation>&lt;b&gt;%1+クリックまたはドラッグ&lt;/b&gt;: クリップするオブジェクトを選択します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/cutout_tool.cpp" line="154"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Clip the selected objects. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 選択中のオブジェクトをクリップします。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DXFParser</name>
    <message>
        <source>Could not open the file.</source>
        <translation type="vanished">ファイルを開くことができませんでした。</translation>
    </message>
    <message>
        <source>The file is not an DXF file.</source>
        <translation type="vanished">ファイルがDXFファイルではありません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DetermineFontSizeDialog</name>
    <message>
        <source>Determine font size</source>
        <translation type="vanished">フォントサイズの決定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="116"/>
        <source>Letter:</source>
        <translation>文字:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="118"/>
        <source>A</source>
        <extracomment>&quot;A&quot; is the default letter which is used for determining letter height.</extracomment>
        <translation>A</translation>
    </message>
    <message>
        <source>This dialog allows to choose a font size which results in a given exact height for a specific letter.</source>
        <translation type="vanished">このダイアログでは、特定の文字の高さを指定してフォントサイズを決めることができます。</translation>
    </message>
    <message>
        <source>mm</source>
        <translation type="vanished">mm</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="124"/>
        <source>Height:</source>
        <translation>高さ:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DistributePointsSettingsDialog</name>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="128"/>
        <source>Distribute points evenly along path</source>
        <translation>パスに沿って均等にポイントを配布する</translation>
    </message>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="134"/>
        <source>Number of points per path:</source>
        <translation>パスあたりのポイント数:</translation>
    </message>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="136"/>
        <source>Also place objects at line end points</source>
        <translation>線の終りのポイントにもオブジェクトを配置する</translation>
    </message>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="142"/>
        <source>Rotation settings</source>
        <translation>回転設定</translation>
    </message>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="145"/>
        <source>Align points with direction of line</source>
        <translation>線の方向でポイントを配置する</translation>
    </message>
    <message>
        <source>°</source>
        <comment>degrees</comment>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/tools/distribute_points_tool.cpp" line="153"/>
        <source>Additional rotation angle (counter-clockwise):</source>
        <translation>追加の回転角度 (反時計回りに):</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawCircleTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to start a circle or ellipse, &lt;b&gt;Drag&lt;/b&gt; to draw a circle</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で円または楕円の描画開始、&lt;b&gt;ドラッグ&lt;/b&gt;で円を描画します</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to draw a circle, &lt;b&gt;Drag&lt;/b&gt; to draw an ellipse, &lt;b&gt;Esc&lt;/b&gt; to abort</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で円を描画、&lt;b&gt;ドラッグ&lt;/b&gt;で楕円を描画、&lt;b&gt;Escキー&lt;/b&gt;で中止します</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="69"/>
        <source>From center</source>
        <comment>Draw circle starting from center</comment>
        <translation>中心から</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="308"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Start a circle or ellipse. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 円または楕円の描画を開始します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="309"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Draw a circle. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 円を描画します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="311"/>
        <source>Hold %1 to start drawing from the center.</source>
        <translation>%1 を押したままにすると、中心から描画を開始します。</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="315"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Finish the circle. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 円の描画を終了します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_circle_tool.cpp" line="316"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Draw an ellipse. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 楕円を描画します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawFreehandTool</name>
    <message>
        <location filename="../src/tools/draw_freehand_tool.cpp" line="289"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Draw a path. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: パスを描画します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawLineAndAreaTool</name>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1160"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="730"/>
        <source>&lt;b&gt;Dash points on.&lt;/b&gt; </source>
        <translation>&lt;b&gt;ダッシュ点を入力します。&lt;/b&gt; </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1168"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt;: Snap or append to existing objects. </source>
        <translation>&lt;b&gt;%1+クリック&lt;/b&gt;: 既存のオブジェクトにスナップ(吸着)またはアペンドします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1176"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="736"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt;: Pick direction from existing objects. </source>
        <translation>&lt;b&gt;%1+クリック&lt;/b&gt;: 既存のオブジェクトから方向をピック。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1177"/>
        <location filename="../src/tools/draw_path_tool.cpp" line="1204"/>
        <source>&lt;b&gt;%1+%2&lt;/b&gt;: Segment azimuth and length. </source>
        <translation>&lt;b&gt;%1+%2&lt;/b&gt;: セグメントの方位角と長さ。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1193"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="741"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="765"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt;: Snap to existing objects. </source>
        <translation>&lt;b&gt;%1+クリック&lt;/b&gt;: 既存のオブジェクトにスナップ(吸着)します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1203"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="755"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Fixed angles. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 一定角度ずつ回転。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1213"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="771"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Undo last point. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 直前のポイントを元に戻す。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawPathTool</name>
    <message>
        <source>&lt;b&gt;Dash points on.&lt;/b&gt; </source>
        <translation type="obsolete">&lt;b&gt;ダッシュ点を入力します。&lt;/b&gt; </translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to start a polygonal segment, &lt;b&gt;Drag&lt;/b&gt; to start a curve</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で折れ線、&lt;b&gt;ドラッグ&lt;/b&gt;で曲線を開始します</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to draw a polygonal segment, &lt;b&gt;Drag&lt;/b&gt; to draw a curve, &lt;b&gt;Right or double click&lt;/b&gt; to finish the path, &lt;b&gt;Return&lt;/b&gt; to close the path, &lt;b&gt;Backspace&lt;/b&gt; to undo, &lt;b&gt;Esc&lt;/b&gt; to abort. Try &lt;b&gt;Space&lt;/b&gt;</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で折れ線、&lt;b&gt;ドラッグ&lt;/b&gt;で曲線を描画、&lt;b&gt;右クリックまたはダブルクリック&lt;/b&gt;でpath を終了、&lt;b&gt;Return キー&lt;/b&gt;でpath を閉じる、&lt;b&gt;Backspace キー&lt;/b&gt;で元に戻す。&lt;b&gt;Esc キー&lt;/b&gt;で中止します。&lt;b&gt;Space キー&lt;/b&gt;でさらに他の機能</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="102"/>
        <source>Finish</source>
        <translation>終了</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="103"/>
        <source>Close</source>
        <translation>閉じる</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="104"/>
        <source>Snap</source>
        <comment>Snap to existing objects</comment>
        <translation>スナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="105"/>
        <source>Angle</source>
        <comment>Using constrained angles</comment>
        <translation>角度</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="106"/>
        <source>Info</source>
        <comment>Show segment azimuth and length</comment>
        <translation>情報</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="109"/>
        <source>Dash</source>
        <comment>Drawing dash points</comment>
        <translation>ダッシュ</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="112"/>
        <source>Undo</source>
        <translation>元に戻す</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="113"/>
        <source>Abort</source>
        <translation>中止</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1155"/>
        <source>&lt;b&gt;Length:&lt;/b&gt; %1 m </source>
        <translation>&lt;b&gt;長さ:&lt;/b&gt; %1 m </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1183"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Start a straight line. &lt;b&gt;Drag&lt;/b&gt;: Start a curve. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 直線を開始。&lt;b&gt;ドラッグ&lt;/b&gt;: 曲線を開始。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1194"/>
        <source>&lt;b&gt;%1+Drag&lt;/b&gt;: Follow existing objects. </source>
        <translation>&lt;b&gt;%1+ドラッグ&lt;/b&gt;: 既存のオブジェクトをフォローします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1210"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Draw a straight line. &lt;b&gt;Drag&lt;/b&gt;: Draw a curve. &lt;b&gt;Right or double click&lt;/b&gt;: Finish the path. &lt;b&gt;%1&lt;/b&gt;: Close the path. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 直線を描く。 &lt;b&gt;ドラッグ&lt;/b&gt;: 曲線を描く。 &lt;b&gt;右クリック/ダブルクリック&lt;/b&gt;: パスを終了。 &lt;b&gt;%1&lt;/b&gt;: パスを閉じる。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawPointGPSTool</name>
    <message>
        <location filename="../src/tools/draw_point_gps_tool.cpp" line="81"/>
        <source>Touch the map to finish averaging</source>
        <translation>地図をタッチすると平均化を終了します</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_gps_tool.cpp" line="193"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Finish setting the object. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: オブジェクトの設定を終了します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawPointTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set a point object, &lt;b&gt;Drag&lt;/b&gt; to set its rotation if the symbol is rotatable</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でポイントオブジェクトを配置、&lt;b&gt;ドラッグ&lt;/b&gt;で回転可能なポイントオブジェクトを回転して配置します</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="83"/>
        <source>Snap</source>
        <comment>Snap to existing objects</comment>
        <translation>スナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="84"/>
        <source>Angle</source>
        <comment>Using constrained angles</comment>
        <translation>角度</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="85"/>
        <source>Reset</source>
        <comment>Reset rotation</comment>
        <translation>リセット</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="334"/>
        <location filename="../src/tools/draw_point_tool.cpp" line="346"/>
        <source>&lt;b&gt;Angle:&lt;/b&gt; %1° </source>
        <translation>&lt;b&gt;角度:&lt;/b&gt; %1° </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="335"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Fixed angles. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 一定角度ずつ回転。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="340"/>
        <location filename="../src/tools/draw_point_tool.cpp" line="354"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Create a point object.</source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: ポイントオブジェクトを作成します。</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="341"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Create an object and set its orientation.</source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: オブジェクトを作成して、方向を設定します。</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_point_tool.cpp" line="347"/>
        <source>&lt;b&gt;%1, 0&lt;/b&gt;: Reset rotation.</source>
        <translation>&lt;b&gt;%1, 0&lt;/b&gt;: 回転をリセットします。</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt;: Create a point object. &lt;b&gt;Drag&lt;/b&gt;: Create an object and set its orientation (if rotatable). </source>
        <translation type="vanished">&lt;b&gt;クリック&lt;/b&gt;: ポイントオブジェクトを作成します。&lt;b&gt;ドラッグ&lt;/b&gt;: ポイントオブジェクトを作成し、方向をセットします(回転可能なオブジェクトのみ)。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawRectangleTool</name>
    <message>
        <source>&lt;b&gt;Dash points on.&lt;/b&gt; </source>
        <translation type="obsolete">&lt;b&gt;ダッシュ点を入力します。&lt;/b&gt; </translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to start drawing a rectangle</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で矩形の描画を開始します</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set a corner point, &lt;b&gt;Right or double click&lt;/b&gt; to finish the rectangle, &lt;b&gt;Backspace&lt;/b&gt; to undo, &lt;b&gt;Esc&lt;/b&gt; to abort</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でコーナー点を配置、&lt;b&gt;右クリックまたはダブルクリック&lt;/b&gt;で矩形の描画を終了、&lt;b&gt;Backspace キー&lt;/b&gt;で元に戻す、&lt;b&gt;Esc キー&lt;/b&gt;で中止します</translation>
    </message>
    <message>
        <source>&lt;b&gt;Space&lt;/b&gt; to toggle dash points</source>
        <translation type="obsolete">&lt;b&gt;Space キー&lt;/b&gt;で、コーナー点とダッシュ点を切り替えます</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="90"/>
        <source>Finish</source>
        <translation>終了</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="91"/>
        <source>Snap</source>
        <comment>Snap to existing objects</comment>
        <translation>スナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="92"/>
        <source>Line snap</source>
        <comment>Snap to previous lines</comment>
        <translation>線のスナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="93"/>
        <source>Dash</source>
        <comment>Drawing dash points</comment>
        <translation>ダッシュ</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="96"/>
        <source>Undo</source>
        <translation>元に戻す</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="97"/>
        <source>Abort</source>
        <translation>中止</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="746"/>
        <source>&lt;b&gt;Click or Drag&lt;/b&gt;: Start drawing a rectangle. </source>
        <translation>&lt;b&gt;クリックまたはドラッグ&lt;/b&gt;: 矩形の描画を開始します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="759"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Snap to previous lines. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 既存のラインにスナップ(吸着)します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="770"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set a corner point. &lt;b&gt;Right or double click&lt;/b&gt;: Finish the rectangle. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: コーナー点を配置します。&lt;b&gt;右クリックまたはダブルクリック&lt;/b&gt;: 矩形の描画を終了します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::DrawTextTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to write text with a single anchor, &lt;b&gt;Drag&lt;/b&gt; to create a text box</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でアンカーポイントを基準にテキストを入力、&lt;b&gt;ドラッグ&lt;/b&gt;でテキストボックスを作成します</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_text_tool.cpp" line="113"/>
        <source>Snap</source>
        <extracomment>Snap to existing objects</extracomment>
        <translation>既存のオブジェクトにスナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_text_tool.cpp" line="476"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Finish editing. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 編集を終了します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_text_tool.cpp" line="477"/>
        <source>&lt;b&gt;%1+%2&lt;/b&gt;: Cancel editing. </source>
        <translation>&lt;b&gt;%1+%2&lt;/b&gt;: 編集をキャンセルします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_text_tool.cpp" line="482"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Create a text object with a single anchor. &lt;b&gt;Drag&lt;/b&gt;: Create a text box. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: シングルアンカーを基準にテキストオブジェクトを作成します。&lt;b&gt;ドラッグ&lt;/b&gt;: テキストボックスを作成します。 </translation>
    </message>
    <message>
        <source>A</source>
        <translation type="vanished">A</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::EditLineTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to select an object, &lt;b&gt;Drag&lt;/b&gt; for box selection, &lt;b&gt;Shift&lt;/b&gt; to toggle selection</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でオブジェクトを選択、&lt;b&gt;ドラッグ&lt;/b&gt;で矩形選択、&lt;b&gt;Shiftキー&lt;/b&gt;で選択を切り替え</translation>
    </message>
    <message>
        <source>, &lt;b&gt;Del&lt;/b&gt; to delete</source>
        <translation type="obsolete">、 &lt;b&gt;Delキー&lt;/b&gt;で削除</translation>
    </message>
    <message>
        <source>; Try &lt;u&gt;Ctrl&lt;/u&gt;</source>
        <translation type="obsolete">、&lt;u&gt;Ctrl キー&lt;/u&gt;でさらに他の機能</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_line_tool.cpp" line="406"/>
        <source>Snap</source>
        <comment>Snap to existing objects</comment>
        <translation>スナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_line_tool.cpp" line="407"/>
        <source>Toggle curve</source>
        <comment>Toggle between curved and flat segment</comment>
        <translation>曲線を切り替え</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_line_tool.cpp" line="519"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Free movement. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 自由に移動します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_line_tool.cpp" line="535"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt; on segment: Toggle between straight and curved. </source>
        <translation>セグメント上を&lt;b&gt;%1+クリック&lt;/b&gt; で、直線と曲線を切り替えます。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::EditPointTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to select an object, &lt;b&gt;Drag&lt;/b&gt; for box selection, &lt;b&gt;Shift&lt;/b&gt; to toggle selection</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でオブジェクトを選択、&lt;b&gt;ドラッグ&lt;/b&gt;で矩形選択、&lt;b&gt;Shiftキー&lt;/b&gt;で選択を切り替え</translation>
    </message>
    <message>
        <source>, &lt;b&gt;Del&lt;/b&gt; to delete</source>
        <translation type="obsolete">、 &lt;b&gt;Delキー&lt;/b&gt;で削除</translation>
    </message>
    <message>
        <source>&lt;b&gt;Ctrl+Click&lt;/b&gt; on point to delete it, on path to add a new point, with &lt;b&gt;Space&lt;/b&gt; to make it a dash point</source>
        <translation type="obsolete">&lt;b&gt;Ctrl キー&lt;/b&gt;を押しながら、ポイントを&lt;b&gt;クリック&lt;/b&gt;で削除、path を&lt;b&gt;クリック&lt;/b&gt;で新しいポイントの追加、さらに&lt;b&gt;Space キー&lt;/b&gt;を押しながらpath を&lt;b&gt;クリック&lt;/b&gt;でダッシュ点を追加します</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="538"/>
        <source>Snap</source>
        <comment>Snap to existing objects</comment>
        <translation>スナップ(吸着)</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="539"/>
        <source>Point / Angle</source>
        <comment>Modify points or use constrained angles</comment>
        <translation>ポイント / 角度</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="540"/>
        <source>Toggle dash</source>
        <comment>Toggle dash points</comment>
        <translation>ダッシュを切り替え</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="719"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Finish editing. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 編集を終了します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="738"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Keep opposite handle positions. </source>
        <extracomment>Actually, this means: &quot;Keep the opposite handle&apos;s position. &quot;</extracomment>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 反対側の方向線(ハンドル)の位置を維持。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="759"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt; on point: Delete it; on path: Add a new dash point; with &lt;b&gt;%2&lt;/b&gt;: Add a normal point. </source>
        <translation>&lt;b&gt;%1+クリック&lt;/b&gt; ポイント上: 削除; パス上: ダッシュポイントを追加; &lt;b&gt;%2&lt;/b&gt;と: 通常の点を追加。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="762"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt; on point: Delete it; on path: Add a new point; with &lt;b&gt;%2&lt;/b&gt;: Add a dash point. </source>
        <translation>&lt;b&gt;%1+クリック&lt;/b&gt; ポイント上: 削除; パス上: ポイントを追加; &lt;b&gt;%2&lt;/b&gt;と: ダッシュ点を追加。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="766"/>
        <source>&lt;b&gt;%1+Click&lt;/b&gt; on point to switch between dash and normal point. </source>
        <translation>ポイント上で&lt;b&gt;%1+クリック&lt;/b&gt;: ダッシュ点と通常の点を切り換え。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::EditTool</name>
    <message>
        <source>&lt;b&gt;Coordinate offset [mm]:&lt;/b&gt; %1, %2  &lt;b&gt;Distance [m]:&lt;/b&gt; %3</source>
        <translation type="obsolete">&lt;b&gt;座標のオフセット [mm]:&lt;/b&gt; %1, %2  &lt;b&gt;距離 [m]:&lt;/b&gt; %3</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to select an object, &lt;b&gt;Drag&lt;/b&gt; for box selection, &lt;b&gt;Shift&lt;/b&gt; to toggle selection</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でオブジェクトを選択、&lt;b&gt;ドラッグ&lt;/b&gt;で矩形選択、&lt;b&gt;Shiftキー&lt;/b&gt;で選択を切り替え</translation>
    </message>
    <message>
        <source>, &lt;b&gt;Del&lt;/b&gt; to delete</source>
        <translation type="obsolete">、 &lt;b&gt;Delキー&lt;/b&gt;で削除</translation>
    </message>
    <message>
        <source>&lt;b&gt;Ctrl+Click&lt;/b&gt; on point to delete it, on path to add a new point, with &lt;b&gt;Space&lt;/b&gt; to make it a dash point</source>
        <translation type="obsolete">&lt;b&gt;Ctrl キー&lt;/b&gt;を押しながら、ポイントを&lt;b&gt;クリック&lt;/b&gt;で削除、path を&lt;b&gt;クリック&lt;/b&gt;で新しいポイントの追加、さらに&lt;b&gt;Space キー&lt;/b&gt;を押しながらpath を&lt;b&gt;クリック&lt;/b&gt;でダッシュ点を追加します</translation>
    </message>
    <message>
        <source>; Try &lt;u&gt;Ctrl&lt;/u&gt;</source>
        <translation type="obsolete">、&lt;u&gt;Ctrl キー&lt;/u&gt;でさらに他の機能</translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="724"/>
        <location filename="../src/tools/edit_line_tool.cpp" line="512"/>
        <source>&lt;b&gt;Coordinate offset:&lt;/b&gt; %1, %2 mm  &lt;b&gt;Distance:&lt;/b&gt; %3 m </source>
        <translation>&lt;b&gt;座標のオフセット:&lt;/b&gt; %1, %2 mm  &lt;b&gt;距離:&lt;/b&gt; %3 m </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="731"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Fixed angles. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 一定の角度上で移動。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_text_tool.cpp" line="484"/>
        <location filename="../src/tools/edit_point_tool.cpp" line="742"/>
        <location filename="../src/tools/edit_line_tool.cpp" line="521"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Snap to existing objects. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 既存のオブジェクトにスナップ(吸着)します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="748"/>
        <location filename="../src/tools/edit_line_tool.cpp" line="527"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Select a single object. &lt;b&gt;Drag&lt;/b&gt;: Select multiple objects. &lt;b&gt;%1+Click&lt;/b&gt;: Toggle selection. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: オブジェクトを選択。 &lt;b&gt;ドラッグ&lt;/b&gt;: 複数のオブジェクトを選択。 &lt;b&gt;%1+クリック&lt;/b&gt;: 選択を切り替え。 </translation>
    </message>
    <message>
        <location filename="../src/tools/edit_point_tool.cpp" line="751"/>
        <location filename="../src/tools/edit_line_tool.cpp" line="530"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Delete selected objects. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 選択中のオブジェクトを削除します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::EditorSettingsPage</name>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="56"/>
        <source>High quality map display (antialiasing)</source>
        <translation>高品質な地図表示 (アンチエイリアス処理)</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="57"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="61"/>
        <source>Antialiasing makes the map look much better, but also slows down the map display</source>
        <translation>アンチエイリアス処理によって地図の外観が滑らかになりますが、表示速度が遅くなります</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="60"/>
        <source>High quality text display in map (antialiasing), slow</source>
        <translation>高品質なテキスト表示 (アンチエイリアス処理)、(表示速度が遅くなります)</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="65"/>
        <source>Click tolerance:</source>
        <translation>クリックの許容誤差:</translation>
    </message>
    <message>
        <source>px</source>
        <translation type="vanished">px</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="49"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="53"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="64"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="67"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="106"/>
        <source>mm</source>
        <comment>millimeters</comment>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="50"/>
        <source>Action button size:</source>
        <translation>アクションボタンのサイズ:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="68"/>
        <source>Snap distance (%1):</source>
        <translation>スナップ（吸着）が有効になる既存オブジェクトからの距離 (%1):</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="73"/>
        <source>Stepping of fixed angle mode (%1):</source>
        <translation>一定角度ずつ回転時のステップ角(%1):</translation>
    </message>
    <message>
        <source>°</source>
        <comment>Degree sign for angles</comment>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="54"/>
        <source>Symbol icon size:</source>
        <translation>記号アイコンサイズ:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="75"/>
        <source>When selecting an object, automatically select its symbol, too</source>
        <translation>オブジェクトを選択する際、同時に記号も選択する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="78"/>
        <source>Zoom away from cursor when zooming out</source>
        <translation>カーソルの位置でズームアウトする</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="81"/>
        <source>Drawing tools: set last point on finishing with right click</source>
        <translation>描画ツール: 右クリックによる終了で最終点をセットする</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="84"/>
        <source>Templates: keep settings of closed templates</source>
        <translation>テンプレート: 閉じたテンプレートの設定を維持する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="89"/>
        <source>Edit tool:</source>
        <translation>編集ツール:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="92"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="98"/>
        <source>Retain old shape</source>
        <translation>以前の形状を保つ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="93"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="99"/>
        <source>Reset outer curve handles</source>
        <translation>方向線(ハンドル)をリセット</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="94"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="100"/>
        <source>Keep outer curve handles</source>
        <translation>方向線(ハンドル)を維持</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="95"/>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="101"/>
        <source>Action on deleting a curve point with %1:</source>
        <translation>%1 でカーブ点を削除したときのふるまい:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="104"/>
        <source>Rectangle tool:</source>
        <translation>矩形ツール:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="107"/>
        <source>Radius of helper cross:</source>
        <translation>十字の補助線の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="109"/>
        <source>Preview the width of lines with helper cross</source>
        <translation>線の太さを十字の補助線でプレビューする</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/editor_settings_page.cpp" line="125"/>
        <source>Editor</source>
        <translation>エディター</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Exporter</name>
    <message>
        <source>Could not create new file: %1</source>
        <translation type="vanished">新規ファイルの作成に失敗しました: %1</translation>
    </message>
    <message>
        <source>Format (%1) does not support export</source>
        <translation type="vanished">形式 (%1) はエクスポートをサポートしていません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="278"/>
        <location filename="../src/fileformats/file_import_export.cpp" line="292"/>
        <location filename="../src/fileformats/file_import_export.cpp" line="306"/>
        <source>Cannot save file
%1:
%2</source>
        <translation>ファイルを保存できません
%1:
%2</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::FillTool</name>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="156"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="157"/>
        <source>The map area is large. Use of the fill tool may be very slow. Do you want to use it anyway?</source>
        <translation>地図エリアが大きいです。塗りつぶしツールの使用は非常に遅くなることがあります。それでも使用しますか?</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="142"/>
        <location filename="../src/tools/fill_tool.cpp" line="176"/>
        <location filename="../src/tools/fill_tool.cpp" line="230"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="143"/>
        <source>The clicked area is not bounded by lines or areas, cannot fill this area.</source>
        <translation>クリックしたエリアは、線またはエリアで閉じられていません、このエリアを塗りつぶしできません。</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="177"/>
        <source>The clicked position is not free, cannot use the fill tool there.</source>
        <translation>クリックした位置は空いていません。塗りつぶしツールを使用できません。</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="231"/>
        <source>Failed to create the fill object.</source>
        <translation>塗りつぶしオブジェクトを作成できませんでした。</translation>
    </message>
    <message>
        <location filename="../src/tools/fill_tool.cpp" line="242"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Fill area with active symbol. The area to be filled must be bounded by lines or areas, other symbols are not taken into account. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 選択中の記号でエリアを塗りつぶします。塗りつぶすエリアは、線またはエリアで閉じている必要があります、他の記号は考慮しません。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Format</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="2065"/>
        <source>In combined symbol %1: Unsupported subsymbol at index %2.</source>
        <translation>結合シンボル %1: インデックス %2 にサポートされていないサブシンボル。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::GPSDisplay</name>
    <message>
        <location filename="../src/sensors/gps_display.cpp" line="164"/>
        <source>GPS is disabled in the device settings. Open settings now?</source>
        <translation>デバイスの設定で GPS は無効です。今すぐ設定を開きますか?</translation>
    </message>
    <message>
        <location filename="../src/sensors/gps_display.cpp" line="165"/>
        <source>Yes</source>
        <translation>はい</translation>
    </message>
    <message>
        <location filename="../src/sensors/gps_display.cpp" line="166"/>
        <source>No</source>
        <translation>いいえ</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::GdalSettingsPage</name>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="53"/>
        <source>Import with GDAL/OGR:</source>
        <translation>GDAL/OGR でインポート:</translation>
    </message>
    <message>
        <source>DXF</source>
        <translation type="vanished">DXF</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="55"/>
        <source>GPX</source>
        <translation>GPX</translation>
    </message>
    <message>
        <source>OSM</source>
        <translation type="vanished">OSM</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="60"/>
        <source>Templates</source>
        <translation>テンプレート</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="62"/>
        <source>Hatch areas</source>
        <translation>ハッチ領域</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="65"/>
        <source>Baseline view</source>
        <translation>ベースライン表示</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="70"/>
        <source>Export Options</source>
        <translation>エクスポートオプション</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="72"/>
        <source>Create a layer for each symbol</source>
        <translation>シンボルごとにレイヤーを作成する</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="77"/>
        <source>Configuration</source>
        <translation>設定</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="85"/>
        <source>Parameter</source>
        <translation>パラメーター</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="85"/>
        <source>Value</source>
        <translation>値</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_settings_page.cpp" line="102"/>
        <source>GDAL/OGR</source>
        <translation>GDAL/OGR</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::GeneralSettingsPage</name>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="77"/>
        <source>Appearance</source>
        <translation>外観</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="82"/>
        <source>Language:</source>
        <translation>言語:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="99"/>
        <source>Screen</source>
        <translation>画面</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="104"/>
        <source>Pixels per inch:</source>
        <translation>インチあたりのピクセル数:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="114"/>
        <source>Program start</source>
        <translation>起動時の表示</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="123"/>
        <source>Saving files</source>
        <translation>保存するファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="125"/>
        <source>Retain compatibility with Mapper %1</source>
        <translation>Mapper %1 と互換性を維持する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="133"/>
        <source>Save undo/redo history</source>
        <translation>元に戻す/やり直しの履歴を保存</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="136"/>
        <source>Save information for automatic recovery</source>
        <translation>自動回復の情報を保存する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="140"/>
        <source>Recovery information saving interval:</source>
        <translation>回復情報の保存間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="139"/>
        <source>min</source>
        <comment>unit minutes</comment>
        <translation>分</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="143"/>
        <source>File import and export</source>
        <translation>ファイルのインポートとエクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="168"/>
        <source>8-bit encoding:</source>
        <translation>8ビットエンコーディング:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="161"/>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="306"/>
        <source>More...</source>
        <translation>さらに表示...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="146"/>
        <source>Default</source>
        <translation>デフォルト</translation>
    </message>
    <message>
        <source>Use the new OCD importer also for version 8 files</source>
        <translation type="vanished">新しい OCD importer でもバージョン8のファイルを使用する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="210"/>
        <source>Notice</source>
        <translation>通知</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="210"/>
        <source>The program must be restarted for the language change to take effect!</source>
        <translation>言語の変更を有効にするには、プログラムを再起動する必要があります!</translation>
    </message>
    <message>
        <source>Use translation file...</source>
        <translation type="vanished">翻訳ファイルを使用...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="348"/>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="354"/>
        <source>Open translation</source>
        <translation>翻訳ファイルを開く</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="348"/>
        <source>Translation files (*.qm)</source>
        <translation>翻訳ファイル(*.qm)</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="355"/>
        <source>The selected file is not a valid translation.</source>
        <translation>選択されたファイルは有効な翻訳ファイルではありません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="385"/>
        <source>%1 x %2</source>
        <translation>%1 x %2</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="386"/>
        <source>Primary screen resolution in pixels:</source>
        <translation>プライマリ画面解像度 (ピクセル):</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="390"/>
        <source>Primary screen size in inches (diagonal):</source>
        <translation>プライマリ画面サイズ インチ (対角):</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/general_settings_page.cpp" line="186"/>
        <source>General</source>
        <translation>全般</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Georeferencing</name>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="92"/>
        <location filename="../src/core/georeferencing.cpp" line="748"/>
        <source>Local coordinates</source>
        <translation>ローカル座標</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="56"/>
        <source>UTM</source>
        <comment>UTM coordinate reference system</comment>
        <translation>UTM</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="57"/>
        <source>UTM coordinates</source>
        <translation>UTM座標</translation>
    </message>
    <message>
        <source>UTM Zone (number north/south, e.g. &quot;32 N&quot;, &quot;24 S&quot;)</source>
        <translation type="vanished">UTMゾーン (数字 North/South 例: &quot;32 N&quot;、&quot;24 S&quot;)</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="60"/>
        <source>UTM Zone (number north/south)</source>
        <translation>UTM ゾーン (番号 北/南)</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="67"/>
        <source>Gauss-Krueger, datum: Potsdam</source>
        <comment>Gauss-Krueger coordinate reference system</comment>
        <translation>ガウス・クリューゲル座標系</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="68"/>
        <source>Gauss-Krueger coordinates</source>
        <translation>ガウス・クリューゲル座標</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="71"/>
        <source>Zone number (1 to 119)</source>
        <comment>Zone number for Gauss-Krueger coordinates</comment>
        <translation>ゾーン番号 (1～119)</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="79"/>
        <source>by EPSG code</source>
        <comment>as in: The CRS is specified by EPSG code</comment>
        <translation>EPSG コードで</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="81"/>
        <source>EPSG @code@ coordinates</source>
        <extracomment>Don&apos;t translate @code@. It is placeholder.</extracomment>
        <translation>EPSG @code@ 座標</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="84"/>
        <source>EPSG code</source>
        <translation>EPSG コード</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="91"/>
        <source>Custom PROJ.4</source>
        <comment>PROJ.4 specification</comment>
        <translation>カスタム PROJ.4</translation>
    </message>
    <message>
        <location filename="../src/core/crs_template_implementation.cpp" line="95"/>
        <source>Specification</source>
        <comment>PROJ.4 specification</comment>
        <translation>規程</translation>
    </message>
    <message>
        <location filename="../src/core/georeferencing.cpp" line="446"/>
        <source>Map scale specification invalid or missing.</source>
        <translation>地図縮尺規程が無効または見つかりません。</translation>
    </message>
    <message>
        <location filename="../src/core/georeferencing.cpp" line="452"/>
        <source>Invalid grid scale factor: %1</source>
        <translation>無効なグリッド尺度: %1</translation>
    </message>
    <message>
        <location filename="../src/core/georeferencing.cpp" line="489"/>
        <location filename="../src/core/georeferencing.cpp" line="516"/>
        <source>Unknown CRS specification language: %1</source>
        <translation>不明な CRS 規程言語: %1</translation>
    </message>
    <message>
        <location filename="../src/core/georeferencing.cpp" line="519"/>
        <source>Unsupported geographic CRS specification: %1</source>
        <translation>地理 CRS 仕様はサポートされていません: %1</translation>
    </message>
    <message>
        <location filename="../src/core/georeferencing.cpp" line="739"/>
        <source>Local</source>
        <translation>ローカル</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::GeoreferencingDialog</name>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="129"/>
        <source>Map Georeferencing</source>
        <translation>マップ ジオリファレンシング</translation>
    </message>
    <message>
        <source>�</source>
        <translation type="obsolete">�</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="207"/>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="538"/>
        <source>Lookup...</source>
        <translation>調べる...</translation>
    </message>
    <message>
        <source>&amp;Select...</source>
        <translation type="obsolete">選択 (&amp;S) ...</translation>
    </message>
    <message>
        <source>Local coordinates</source>
        <translation type="obsolete">ローカル座標</translation>
    </message>
    <message>
        <source>Edit projection parameters...</source>
        <translation type="obsolete">投影パラメータを編集...</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="166"/>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="167"/>
        <source>m</source>
        <translation>m</translation>
    </message>
    <message>
        <source>General</source>
        <translation type="obsolete">一般</translation>
    </message>
    <message>
        <source>Map scale:</source>
        <translation type="obsolete">地図の縮尺:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="241"/>
        <source>Declination:</source>
        <translation>磁気偏角:</translation>
    </message>
    <message>
        <source>Reference point:</source>
        <translation type="obsolete">基準点:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="192"/>
        <source>Projected coordinates</source>
        <translation>投影座標</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="224"/>
        <location filename="../src/gui/select_crs_dialog.cpp" line="93"/>
        <source>&amp;Coordinate reference system:</source>
        <translation>座標参照系 (&amp;C) :</translation>
    </message>
    <message>
        <source>&amp;Zone:</source>
        <translation type="obsolete">&amp;Zone:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="138"/>
        <source>Status:</source>
        <translation>状態:</translation>
    </message>
    <message>
        <source>Reference point &amp;easting:</source>
        <translation type="obsolete">基準点の偏東距離 (&amp;E):</translation>
    </message>
    <message>
        <source>Reference point &amp;northing:</source>
        <translation type="obsolete">基準点の偏北距離 (&amp;N):</translation>
    </message>
    <message>
        <source>Convergence:</source>
        <translation type="obsolete">コンバージェンス:</translation>
    </message>
    <message>
        <source>&amp;Grivation:</source>
        <translation type="obsolete">グリッド偏差(&amp;G):</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="193"/>
        <source>Geographic coordinates</source>
        <translation>地理座標</translation>
    </message>
    <message>
        <source>Datum</source>
        <translation type="obsolete">測地系</translation>
    </message>
    <message>
        <source>Reference point &amp;latitude:</source>
        <translation type="obsolete">基準点の緯度 (&amp;L):</translation>
    </message>
    <message>
        <source>Reference point longitude:</source>
        <translation type="obsolete">基準点の経度:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="188"/>
        <source>Show reference point in:</source>
        <translation>参照点を表示:</translation>
    </message>
    <message>
        <source>%1 %2 (mm)</source>
        <translation type="obsolete">%1 %2 (mm)</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="376"/>
        <source>valid</source>
        <translation>有効</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="370"/>
        <source>&lt;a href=&quot;%1&quot;&gt;OpenStreetMap&lt;/a&gt; | &lt;a href=&quot;%2&quot;&gt;World of O Maps&lt;/a&gt;</source>
        <translation>&lt;a href=&quot;%1&quot;&gt;OpenStreetMap&lt;/a&gt; | &lt;a href=&quot;%2&quot;&gt;World of O Maps&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="400"/>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="713"/>
        <source>Online declination lookup</source>
        <translation>オンラインで磁気偏角を調べる</translation>
    </message>
    <message>
        <source>The magnetic declination for the reference point %1� %2� will now be retrieved from &lt;a href=&quot;%3&quot;&gt;%3&lt;/a&gt;. Do you want to continue?</source>
        <translation type="obsolete">基準点 %1� %2� の磁気偏角を &lt;a href=&quot;%3&quot;&gt;%3&lt;/a&gt; から検索します。続行しますか？</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="133"/>
        <source>Map coordinate reference system</source>
        <translation>地図の座標参照系</translation>
    </message>
    <message>
        <source>- none -</source>
        <translation type="vanished">- none -</translation>
    </message>
    <message>
        <source>- from Proj.4 specification -</source>
        <translation type="vanished">- Proj.4 仕様から -</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="136"/>
        <source>- local -</source>
        <translation>- local -</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="144"/>
        <source>Grid scale factor:</source>
        <extracomment>The grid scale factor is the ratio between a length in the grid plane and the corresponding length on the curved earth model. It is applied as a factor to ground distances to get grid plane distances.</extracomment>
        <translation>グリッド尺度係数:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="147"/>
        <source>Reference point</source>
        <translation>参照点</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="149"/>
        <source>&amp;Pick on map</source>
        <translation>地図から選択(&amp;P)</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="151"/>
        <source>(Datum: WGS84)</source>
        <translation>(データム: WGS84)</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="154"/>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="155"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="159"/>
        <source>X</source>
        <comment>x coordinate</comment>
        <translation>X</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="161"/>
        <source>Y</source>
        <comment>y coordinate</comment>
        <translation>Y</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="170"/>
        <source>E</source>
        <comment>west / east</comment>
        <translation>E</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="172"/>
        <source>N</source>
        <comment>north / south</comment>
        <translation>N</translation>
    </message>
    <message>
        <source>°</source>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="181"/>
        <source>N</source>
        <comment>north</comment>
        <translation>N</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="183"/>
        <source>E</source>
        <comment>east</comment>
        <translation>E</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="204"/>
        <source>Map north</source>
        <translation>地図上の北の向き</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="231"/>
        <source>Map coordinates:</source>
        <translation>地図上の座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="233"/>
        <source>Geographic coordinates:</source>
        <translation>地理座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="236"/>
        <source>On CRS changes, keep:</source>
        <translation>座標参照系の変更時に維持する座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="242"/>
        <source>Grivation:</source>
        <translation>グリッド偏差:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="401"/>
        <source>The magnetic declination for the reference point %1° %2° will now be retrieved from &lt;a href=&quot;%3&quot;&gt;%3&lt;/a&gt;. Do you want to continue?</source>
        <translation>参照点 &quot;%1&quot; &quot;%2&quot; の磁気偏角を &lt;a href=&quot;%3&quot;&gt;%3&lt;/a&gt; から検索します。続行しますか？</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="481"/>
        <source>Declination change</source>
        <translation>赤緯の変更</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="481"/>
        <source>The declination has been changed. Do you want to rotate the map content accordingly, too?</source>
        <translation>赤緯が変更されました。地図コンテンツもしたがって回転しますか?</translation>
    </message>
    <message>
        <source>Projected coordinates:</source>
        <translation type="vanished">投影座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="511"/>
        <source>Local coordinates:</source>
        <translation>ローカル座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="538"/>
        <source>Loading...</source>
        <translation>読み込み...</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="545"/>
        <source>locked</source>
        <translation>ロック済</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="684"/>
        <source>Could not parse data.</source>
        <translation>データを解析できませんでした。</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="709"/>
        <source>Declination value not found.</source>
        <translation>磁気偏角の値が見つかりませんでした。</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="714"/>
        <source>The online declination lookup failed:
%1</source>
        <translation>オンラインで磁気偏角を調べることに失敗しました:
%1</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="543"/>
        <source>%1 °</source>
        <comment>degree value</comment>
        <translation>%1 °</translation>
    </message>
    <message>
        <source>Custom coordinates</source>
        <translation type="obsolete">カスタム座標</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::GeoreferencingTool</name>
    <message>
        <source>&lt;b&gt;Left click&lt;/b&gt; to set the reference point, another button to cancel</source>
        <translation type="obsolete">&lt;b&gt;左クリック&lt;/b&gt;で参照点をセット、右クリックでキャンセルします</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the reference point. Another button to cancel.</source>
        <translation type="vanished">&lt;b&gt;クリック&lt;/b&gt;: 参照点をセットします。他のボタンでキャンセルします。</translation>
    </message>
    <message>
        <location filename="../src/gui/georeferencing_dialog.cpp" line="742"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the reference point. &lt;b&gt;Right click&lt;/b&gt;: Cancel.</source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 参照点を設定します。&lt;b&gt;右クリック&lt;/b&gt;: キャンセルします。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Global</name>
    <message>
        <source>OpenOrienteering Mapper</source>
        <translation type="obsolete">OpenOrienteering Mapper</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::HomeScreenController</name>
    <message>
        <location filename="../src/gui/home_screen_controller.cpp" line="149"/>
        <source>Welcome to OpenOrienteering Mapper!</source>
        <translation>OpenOrienteering Mapper へようこそ！</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::HomeScreenWidget</name>
    <message>
        <source>Clear list</source>
        <translation type="vanished">リストをクリア</translation>
    </message>
    <message>
        <source>Exit</source>
        <translation type="vanished">終了</translation>
    </message>
    <message>
        <source>Activities</source>
        <translation type="vanished">アクティビティ</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation type="vanished">設定</translation>
    </message>
    <message>
        <source>About %1</source>
        <translation type="obsolete">%1 について</translation>
    </message>
    <message>
        <source>Help</source>
        <translation type="vanished">ヘルプ</translation>
    </message>
    <message>
        <source>Open most recently used file on start</source>
        <translation type="vanished">直前に使用したファイルを起動時に開く</translation>
    </message>
    <message>
        <source>Tip of the day</source>
        <translation type="vanished">今日のヒント</translation>
    </message>
    <message>
        <source>Previous</source>
        <translation type="vanished">前へ</translation>
    </message>
    <message>
        <source>Next</source>
        <translation type="vanished">次へ</translation>
    </message>
    <message>
        <source>Maps</source>
        <translation type="obsolete">地図</translation>
    </message>
    <message>
        <source>Create a new map ...</source>
        <translation type="vanished">新しい地図を作成...</translation>
    </message>
    <message>
        <source>Open map ...</source>
        <translation type="vanished">地図を開く...</translation>
    </message>
    <message>
        <source>About %1</source>
        <comment>As in &apos;About OpenOrienteering Mapper&apos;</comment>
        <translation type="vanished">%1 について</translation>
    </message>
    <message>
        <source>Recent maps</source>
        <translation type="vanished">最近開いた地図</translation>
    </message>
    <message>
        <source>Maps (*.omap *.ocd);;All files (*.*)</source>
        <translation type="obsolete">地図ファイル (*.omap *.ocd);;すべてのファイル (*.*)</translation>
    </message>
    <message>
        <source>Open most recently used file</source>
        <translation type="vanished">直前に使用したファイルを開く</translation>
    </message>
    <message>
        <source>Show tip of the day</source>
        <translation type="vanished">今日のヒントを表示する</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::HomeScreenWidgetDesktop</name>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="146"/>
        <source>Activities</source>
        <translation>アクティビティ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="149"/>
        <source>Create a new map ...</source>
        <translation>新しい地図を作成 ...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="152"/>
        <source>Open map ...</source>
        <translation>地図を開く ...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="157"/>
        <source>Touch mode</source>
        <translation>タッチモード</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="162"/>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="347"/>
        <source>Settings</source>
        <translation>設定</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="165"/>
        <source>About %1</source>
        <comment>As in &apos;About OpenOrienteering Mapper&apos;</comment>
        <translation>%1 について</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="168"/>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="349"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="171"/>
        <source>Exit</source>
        <translation>終了</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="192"/>
        <source>Recent maps</source>
        <translation>最近開いた地図</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="217"/>
        <source>Open most recently used file on start</source>
        <translation>直前に使用したファイルを起動時に開く</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="220"/>
        <source>Clear list</source>
        <translation>リストをクリア</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="239"/>
        <source>Tip of the day</source>
        <translation>今日のヒント</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="244"/>
        <source>Show tip of the day</source>
        <translation>今日のヒントを表示する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="248"/>
        <source>Previous</source>
        <translation>前へ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="250"/>
        <source>Next</source>
        <translation>次へ</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::HomeScreenWidgetMobile</name>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="533"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="554"/>
        <source>Examples</source>
        <translation>例</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="351"/>
        <source>About Mapper</source>
        <translation>Mapper について</translation>
    </message>
    <message>
        <source>File list</source>
        <translation type="vanished">ファイルリスト</translation>
    </message>
    <message>
        <source>No map files found!&lt;br/&gt;&lt;br/&gt;Copy map files to a top-level folder named &apos;OOMapper&apos; on the device or a memory card.</source>
        <translation type="vanished">地図ファイルが見つかりません!&lt;br/&gt;&lt;br/&gt;デバイスやメモリカードの &apos;OOMapper&apos; という名前の最上位フォルダーに地図ファイルをコピーしてください。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::IconPropertiesWidget</name>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="48"/>
        <source>PNG</source>
        <translation>PNG</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="49"/>
        <source>All files (*.*)</source>
        <translation>すべてのファイル (*.*)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="67"/>
        <source>Default icon</source>
        <translation>デフォルトのアイコン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="69"/>
        <source>px</source>
        <translation>px</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="70"/>
        <source>Preview width:</source>
        <translation>プレビュー幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="86"/>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="118"/>
        <source>Save...</source>
        <translation>保存...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="88"/>
        <source>Copy to custom icon</source>
        <translation>カスタムアイコンにコピー</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="98"/>
        <source>Custom icon</source>
        <translation>カスタムアイコン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="102"/>
        <source>Width:</source>
        <translation>幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="120"/>
        <source>Load...</source>
        <translation>読み込み...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="122"/>
        <source>Clear</source>
        <translation>クリア</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="169"/>
        <source>%1 px</source>
        <translation>%1 px</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="203"/>
        <source>Save symbol icon ...</source>
        <translation>シンボルアイコンを保存 ...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="217"/>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="234"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="218"/>
        <source>Failed to save the image:
%1</source>
        <translation>画像の保存に失敗しました:
%1</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="226"/>
        <source>Load symbol icon ...</source>
        <translation>記号アイコンを読み込む ...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/icon_properties_widget.cpp" line="235"/>
        <source>Cannot open file:
%1

%2</source>
        <translation>ファイルを開くことができません:
%1

%2</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ImportExport</name>
    <message>
        <location filename="../src/core/symbols/symbol.cpp" line="297"/>
        <source>Error while loading a symbol of type %1 at line %2 column %3.</source>
        <translation>%2 行目 %3 列目で、タイプ %1 の記号を読み込み中にエラー。</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/symbol.cpp" line="312"/>
        <source>Symbol ID &apos;%1&apos; not unique at line %2 column %3.</source>
        <translation>%2 行目 %3 列目で、記号 ID &apos;%1&apos; は一意ではありません。</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/symbol.cpp" line="392"/>
        <source>Error while loading a symbol of type %1 at line %2 column %3: %4</source>
        <translation>%2 行目 %3 列目で、タイプ %1 の記号を読み込み中にエラー: %4</translation>
    </message>
    <message>
        <source>Error while loading a symbol.</source>
        <translation type="vanished">記号を読み込む際にエラーが発生しました。</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object.cpp" line="315"/>
        <source>Error while loading an object of type %1.</source>
        <translation>タイプ %1 のオブジェクトを読み込み中にエラー。</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object.cpp" line="327"/>
        <location filename="../src/core/symbols/symbol.cpp" line="318"/>
        <location filename="../src/undo/object_undo.cpp" line="616"/>
        <source>Malformed symbol ID &apos;%1&apos; at line %2 column %3.</source>
        <translation type="unfinished">行％2、列％3に不正なシンボルID &apos;％1&apos;があります。</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object.cpp" line="389"/>
        <source>Error while loading an object of type %1 at %2:%3: %4</source>
        <translation>%2:%3 で、タイプ %1 のオブジェクトを読み込み中にエラー: %4</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object.cpp" line="353"/>
        <source>Unable to find symbol for object at %1:%2.</source>
        <translation>%1:%2 で、オブジェクトの記号を見つけることができません。</translation>
    </message>
    <message>
        <source>Point object with undefined or wrong symbol at %1:%2.</source>
        <translation type="vanished">%1:%2 で、定義されていないか間違った記号のポイントオブジェクト。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="93"/>
        <source>OpenOrienteering Mapper</source>
        <translation>OpenOrienteering Mapper</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="65"/>
        <source>OCAD Versions 7, 8</source>
        <translation>OCAD バージョン 7, 8</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_format.cpp" line="46"/>
        <source>OCAD</source>
        <translation>OCAD</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_format.cpp" line="48"/>
        <source>OCAD version 8, old implementation</source>
        <translation>OCAD バージョン 8,旧実装</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_format.cpp" line="50"/>
        <source>OCAD version %1</source>
        <translation>OCAD バージョン %1</translation>
    </message>
    <message>
        <location filename="../src/util/xml_stream_util.cpp" line="225"/>
        <location filename="../src/util/xml_stream_util.cpp" line="241"/>
        <location filename="../src/util/xml_stream_util.cpp" line="288"/>
        <location filename="../src/util/xml_stream_util.cpp" line="310"/>
        <source>Could not parse the coordinates.</source>
        <translation>座標を解析できませんでした。</translation>
    </message>
    <message>
        <location filename="../src/util/xml_stream_util.cpp" line="265"/>
        <location filename="../src/util/xml_stream_util.cpp" line="340"/>
        <source>Expected %1 coordinates, found %2.</source>
        <translation>%1 の座標が必要ですが、%2 が見つかりました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="1048"/>
        <source>Error while loading the printing configuration at %1:%2: %3</source>
        <translation>%1:%2 で、印刷の設定を読み込み中にエラー: %3</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="1067"/>
        <location filename="../src/fileformats/xml_file_format.cpp" line="1087"/>
        <source>Error while loading the undo/redo steps at %1:%2: %3</source>
        <translation>%1:%2 で、ステップを元に戻す/繰り返しを読み込み中にエラー: %3</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="69"/>
        <source>No such option: %1</source>
        <comment>No such import / export option</comment>
        <translation>そのようなオプションはありません: %1</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="504"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="523"/>
        <source>Geospatial vector data</source>
        <translation>空間ベクトル データ</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Importer</name>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="158"/>
        <source>Found an object without symbol.</source>
        <translation>記号のないオブジェクトが見つかりました。</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/fileformats/file_import_export.cpp" line="186"/>
        <source>Dropped %n irregular object(s).</source>
        <translation>
            <numerusform>不規則な %n のオブジェクトを削除します。</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="193"/>
        <source>Error during symbol post-processing.</source>
        <translation>記号の後処理中にエラー。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="226"/>
        <source>Template &quot;%1&quot; has been loaded from the map&apos;s directory instead of the relative location to the map file where it was previously.</source>
        <translation>テンプレート&quot;%1&quot;は、以前あった地図ファイルへの相対的な位置ではなく、地図のディレクトリから読み込まれています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="217"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="750"/>
        <source>Failed to load template &apos;%1&apos;, reason: %2</source>
        <translation>テンプレート &apos;%1&apos; の読み込みに失敗しました。理由: %2</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="104"/>
        <location filename="../src/fileformats/file_import_export.cpp" line="123"/>
        <source>Cannot open file
%1:
%2</source>
        <translation>ファイルを開くことができません
%1:
%2</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="234"/>
        <source>Warnings when loading template &apos;%1&apos;:
%2</source>
        <translation>テンプレート &apos;%1&apos; の読み込み時に警告:
%2</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="242"/>
        <location filename="../src/fileformats/file_import_export.cpp" line="244"/>
        <source>At least one template file could not be found.</source>
        <translation>テンプレート ファイルが少なくとも 1 つ見つかりませんでした。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/file_import_export.cpp" line="245"/>
        <source>Click the red template name(s) in the Templates -&gt; Template setup window to locate the template file name(s).</source>
        <translation>テンプレート -&gt; テンプレートウィンドウで、赤のテンプレート名をクリックして、テンプレートファイル名を設定します。</translation>
    </message>
    <message>
        <source>This file uses an obsolete format. Support for this format is to be removed from this program soon. To be able to open the file in the future, save it again.</source>
        <translation type="vanished">このファイルは、古いバージョンの形式を使用します。この形式のサポートは間もなくこのプログラムから削除されます。今後、ファイルを開くようにするには、再度保存してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="546"/>
        <source>Unsupported obsolete file format version. Please use program version v%1 or older to load and update the file.</source>
        <translation>サポートされていない古いバージョンのファイル形式です。プログラムバージョン v%1 以上を使用して、ファイルを読み込んで更新してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="558"/>
        <source>Invalid file format version.</source>
        <translation>無効なバージョンのファイル形式です。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="560"/>
        <source>Unsupported old file format version. Please use an older program version to load and update the file.</source>
        <translation>サポートされていない古いファイル形式のバージョンです。ファイルの読み込みと更新には古いバージョンのプログラムを使用してください。</translation>
    </message>
    <message>
        <source>Unsupported new file format version. Some map features will not be loaded or saved by this version of the program. Consider updating.</source>
        <translation type="vanished">サポートされていない新しいファイル形式のバージョンです。このバージョンのプログラムでは、いくつかの地図機能を読み込めないか、保存されません。更新をご検討ください。</translation>
    </message>
    <message>
        <source>The geographic coordinate reference system of the map was &quot;%1&quot;. This CRS is not supported. Using &quot;%2&quot;.</source>
        <translation type="vanished">この地図の地理座標参照系は &quot;%1&quot; です。この地理座標参照系はサポートされていません。&quot;%2&quot; を使用してください。</translation>
    </message>
    <message>
        <source>Error while loading a symbol with type %2.</source>
        <translation type="vanished">%2 のタイプの記号を読み込む際にエラーが発生しました。</translation>
    </message>
    <message>
        <source>Error while loading a symbol.</source>
        <translation type="vanished">記号を読み込む際にエラーが発生しました。</translation>
    </message>
    <message>
        <source>Error while loading undo steps.</source>
        <translation type="vanished">アンドゥ履歴を読み込む際にエラーが発生しました。</translation>
    </message>
    <message>
        <source>Error while reading map part count.</source>
        <translation type="vanished">地図パート数を読み取り中にエラー。</translation>
    </message>
    <message>
        <source>Error while loading map part %2.</source>
        <translation type="vanished">地図パート %2 を読み込み中にエラー。</translation>
    </message>
    <message>
        <source>Could not read file: %1</source>
        <translation type="vanished">ファイルの読み込みに失敗しました: %1</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="552"/>
        <source>Unsupported file format.</source>
        <translation>サポートされていないファイル形式です。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="562"/>
        <source>Unsupported new file format version. Some map features will not be loaded or saved by this version of the program.</source>
        <translation>サポートされていない新しいファイル形式のバージョンです。このバージョンのプログラムでは、いくつかの地図機能を読み込めないか、保存されません。</translation>
    </message>
    <message>
        <source>Format (%1) does not support import</source>
        <translation type="vanished">形式 (%1) はインポートをサポートしていません</translation>
    </message>
    <message>
        <source>Could not read &apos;%1&apos;: %2</source>
        <translation type="vanished">&apos;%1&apos; を読み込みできません: %2</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::LineSymbolSettings</name>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="320"/>
        <source>Line settings</source>
        <translation>ライン記号の設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="81"/>
        <source>Line width:</source>
        <translation>ラインの幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="84"/>
        <source>Line color:</source>
        <translation>ラインの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="96"/>
        <source>Minimum line length:</source>
        <translation>最小のラインの長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="105"/>
        <source>Line cap:</source>
        <translation>ラインのキャップ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="107"/>
        <source>flat</source>
        <translation>平ら(flat)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="102"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="108"/>
        <source>round</source>
        <translation>丸型(round)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="109"/>
        <source>square</source>
        <translation>突出線端(square)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="110"/>
        <source>pointed</source>
        <translation>三角(pointed)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="99"/>
        <source>Line join:</source>
        <translation>角の形状:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="101"/>
        <source>miter</source>
        <translation>マイター結合(miter)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="103"/>
        <source>bevel</source>
        <translation>ベベル結合(bevel)</translation>
    </message>
    <message>
        <source>Cap length:</source>
        <translation type="vanished">キャップの長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="118"/>
        <source>Line is dashed</source>
        <translation>ラインを破線にする</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="220"/>
        <source>Show at least one mid symbol</source>
        <translation>中間記号を一つ以上表示する</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="222"/>
        <source>Minimum mid symbol count:</source>
        <translation>最小の中間記号数:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="225"/>
        <source>Minimum mid symbol count when closed:</source>
        <translation>ラインを閉じる場合の、最小の中間記号数:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="154"/>
        <source>Dash length:</source>
        <translation>実線部分の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="82"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="97"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="113"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="116"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="155"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="158"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="168"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="212"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="215"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="218"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="566"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="572"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="597"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="600"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="150"/>
        <source>Dashed line</source>
        <translation>破線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="157"/>
        <source>Break length:</source>
        <translation>空白部分の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="160"/>
        <source>Dashes grouped together:</source>
        <translation>破線のグループ化:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="162"/>
        <source>none</source>
        <translation>none</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="163"/>
        <source>2</source>
        <translation>2</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="164"/>
        <source>3</source>
        <translation>3</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="165"/>
        <source>4</source>
        <translation>4</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="167"/>
        <source>In-group break length:</source>
        <translation>グループ内の空白部分の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="170"/>
        <source>Half length of first and last dash</source>
        <translation>両端の実線部分の長さを半分にする</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="172"/>
        <source>Mid symbols placement:</source>
        <translation>中間記号の配置:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="174"/>
        <source>Center of dashes</source>
        <translation>ダッシュの中心</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="175"/>
        <source>Center of dash groups</source>
        <translation>ダッシュグループの中心</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="176"/>
        <source>Center of gaps</source>
        <translation>ギャップの中心</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="206"/>
        <source>Mid symbols</source>
        <translation>中間記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="208"/>
        <source>Mid symbols per spot:</source>
        <translation>中間記号のスポットごとの数:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="211"/>
        <source>Mid symbol distance:</source>
        <translation>中間記号間の距離:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="269"/>
        <source>Borders</source>
        <translation>輪郭線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="273"/>
        <source>Different borders on left and right sides</source>
        <translation>左右で異なる輪郭線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="277"/>
        <source>Left border:</source>
        <translation>左輪郭線:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="282"/>
        <source>Right border:</source>
        <translation>右輪郭線:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="302"/>
        <source>Suppress the dash symbol at line start and line end</source>
        <translation>線の開始と終了でダッシュ記号を抑制する</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="306"/>
        <source>Scale the dash symbol at corners</source>
        <translation>角にダッシュ記号のスケール</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="677"/>
        <source>Cap length at start:</source>
        <translation>開始時のキャップ長:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="678"/>
        <source>Cap length at end:</source>
        <translation>終端のキャップ長:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="682"/>
        <source>Offset at start:</source>
        <translation>開始時のオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="683"/>
        <source>Offset at end:</source>
        <translation>終端のオフセット:</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/line_symbol.cpp" line="1725"/>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="620"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1503"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="325"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="815"/>
        <source>Start symbol</source>
        <translation>開始記号</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/line_symbol.cpp" line="1729"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="325"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="815"/>
        <source>Mid symbol</source>
        <translation>中間記号</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/line_symbol.cpp" line="1733"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1510"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="325"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="815"/>
        <source>End symbol</source>
        <translation>終了記号</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/line_symbol.cpp" line="1737"/>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="614"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1496"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="300"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="325"/>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="815"/>
        <source>Dash symbol</source>
        <translation>ダッシュ記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="120"/>
        <source>Enable border lines</source>
        <translation>輪郭線を有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="214"/>
        <source>Distance between spots:</source>
        <translation>スポットの間の距離:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="217"/>
        <source>Distance from line end:</source>
        <translation>ラインの端からの距離:</translation>
    </message>
    <message>
        <source>Border</source>
        <translation type="obsolete">輪郭線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="565"/>
        <source>Border width:</source>
        <translation>輪郭線の幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="568"/>
        <source>Border color:</source>
        <translation>輪郭線の色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="571"/>
        <source>Border shift:</source>
        <translation>輪郭線のシフト距離:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="574"/>
        <source>Border is dashed</source>
        <translation>輪郭線を破線にする</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="596"/>
        <source>Border dash length:</source>
        <translation>輪郭線の実線部分の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/line_symbol_settings.cpp" line="599"/>
        <source>Border break length:</source>
        <translation>輪郭線の空白部分の長さ:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MainWindow</name>
    <message>
        <location filename="../src/gui/main_window.cpp" line="329"/>
        <source>&amp;New</source>
        <translation>新規(&amp;N)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="332"/>
        <source>Create a new map</source>
        <translation>新しい地図を作成</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="336"/>
        <source>&amp;Open...</source>
        <translation>開く(&amp;O)...</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="339"/>
        <source>Open an existing file</source>
        <translation>既存のファイルを開く</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="343"/>
        <source>Open &amp;recent</source>
        <translation>最近開いたファイル(&amp;R)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="355"/>
        <source>&amp;Save</source>
        <translation>上書き保存(&amp;S)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="361"/>
        <source>Save &amp;as...</source>
        <translation>名前を付けて保存(&amp;A)...</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="364"/>
        <source>Ctrl+Shift+S</source>
        <translation>Ctrl+Shift+S</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="370"/>
        <source>Settings...</source>
        <translation>設定...</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="375"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="136"/>
        <source>Close</source>
        <translation>閉じる</translation>
    </message>
    <message>
        <source>Ctrl+W</source>
        <translation type="obsolete">Ctrl+W</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="378"/>
        <source>Close this file</source>
        <translation>ファイルを閉じる</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="382"/>
        <source>E&amp;xit</source>
        <translation>終了(&amp;X)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="385"/>
        <source>Exit the application</source>
        <translation>アプリケーションを終了</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="391"/>
        <source>&amp;File</source>
        <translation>ファイル(&amp;F)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="410"/>
        <source>General</source>
        <translation>一般</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="425"/>
        <source>Open &amp;Manual</source>
        <translation>マニュアルを開く(&amp;M)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="427"/>
        <source>Show the help file for this application</source>
        <translation>このアプリケーションのヘルプファイルを表示</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="431"/>
        <source>&amp;About %1</source>
        <translation>%1 について(&amp;A)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="433"/>
        <source>Show information about this application</source>
        <translation>このアプリケーションについての情報を表示</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="436"/>
        <source>About &amp;Qt</source>
        <translation>QT について(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="438"/>
        <source>Show information about Qt</source>
        <translation>Qtについての情報を表示</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="443"/>
        <source>&amp;Help</source>
        <translation>ヘルプ(&amp;H)</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="649"/>
        <source>Do you want to remove the autosaved version?</source>
        <translation>自動保存したバージョンを削除しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="655"/>
        <source>The file has been modified.
Do you want to save your changes?</source>
        <translation>ファイルが変更されています。
変更を保存しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="470"/>
        <location filename="../src/gui/main_window.cpp" line="812"/>
        <source>Unsaved file</source>
        <translation>未保存のファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="770"/>
        <location filename="../src/gui/main_window.cpp" line="858"/>
        <source>Cannot open file:
%1

%2</source>
        <translation>ファイルを開くことができません:
%1

%2</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="776"/>
        <location filename="../src/gui/main_window.cpp" line="780"/>
        <location filename="../src/gui/main_window.cpp" line="1114"/>
        <location filename="../src/gui/widgets/home_screen_widget.cpp" line="454"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="776"/>
        <source>The symbol set import generated warnings.</source>
        <translation>記号セットのインポートで警告が生成されました。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="780"/>
        <source>The selected map scale is 1:%1, but the chosen symbol set has a nominal scale of 1:%2.

Do you want to scale the symbols to the selected scale?</source>
        <translation>選択中の地図の縮尺は1:%1です。しかし選択された記号セットの名目上の縮尺は1:%2 です。
記号の縮尺を地図の縮尺に合わせますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="859"/>
        <source>Invalid file type.</source>
        <translation>無効なファイル形式です。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="868"/>
        <source>Crash warning</source>
        <translation>クラッシュ警告</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="869"/>
        <source>It seems that %1 crashed the last time this file was opened:&lt;br /&gt;&lt;tt&gt;%2&lt;/tt&gt;&lt;br /&gt;&lt;br /&gt;Really retry to open it?</source>
        <translation>最後にこのファイルを開いた時に %1 はクラッシュしたようです:&lt;br /&gt;&lt;tt&gt;%2&lt;/tt&gt;&lt;br /&gt;&lt;br /&gt;もう一度開いてもよろしいですか?</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1068"/>
        <source>Autosaving...</source>
        <translation>自動保存中...</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1078"/>
        <source>Autosaving failed!</source>
        <translation>自動保存に失敗しました!</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1167"/>
        <source>All maps</source>
        <translation>すべての地図ファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1171"/>
        <source>All files</source>
        <translation>すべてのファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="823"/>
        <source>Open file</source>
        <translation>ファイルを開く</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="175"/>
        <source>You must close the current file before you can open another one.</source>
        <translation>別のファイルを開く前に、現在のファイルを閉じる必要があります。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="844"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="737"/>
        <source>Opening %1</source>
        <translation>%1 を開きます</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="769"/>
        <location filename="../src/gui/main_window.cpp" line="857"/>
        <location filename="../src/gui/main_window.cpp" line="885"/>
        <location filename="../src/gui/main_window.cpp" line="1259"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="749"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="885"/>
        <source>Cannot open file:
%1

File format not recognized.</source>
        <translation>ファイルを開くことができません。
%1

ファイル形式を認識できません。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="965"/>
        <source>The file has been modified.
Do you want to discard your changes?</source>
        <translation>ファイルが変更されています。
変更を破棄しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1020"/>
        <source>&amp;%1 %2</source>
        <translation>&amp;%1 %2</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1109"/>
        <source>This map is being saved as a &quot;%1&quot; file. Information may be lost.

Press Yes to save in this format.
Press No to choose a different format.</source>
        <translation>地図は&quot;%1&quot;として保存されます。情報が失われます。

Yesを押してこの形式で保存します。
Noを押して形式を変更します。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1239"/>
        <source>Save file</source>
        <translation>ファイルを保存</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1260"/>
        <source>File could not be saved:</source>
        <translation>ファイルを保存することができません:</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1261"/>
        <source>There was a problem in determining the file format.</source>
        <translation>ファイル形式を決定するときに問題が発生しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window.cpp" line="1262"/>
        <source>Please report this as a bug.</source>
        <translation>この問題をバグとして報告してください。</translation>
    </message>
    <message>
        <source>This program uses the &lt;b&gt;Clipper library&lt;/b&gt; by Angus Johnson.</source>
        <translation type="obsolete">This program uses the &lt;b&gt;Clipper library&lt;/b&gt; by Angus Johnson.</translation>
    </message>
    <message>
        <source>See &lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt; for more information.</source>
        <translation type="obsolete">See &lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt; for more information.</translation>
    </message>
    <message>
        <source>This program uses the &lt;b&gt;PROJ.4 Cartographic Projections Library&lt;/b&gt; by Frank Warmerdam.</source>
        <translation type="obsolete">This program uses the &lt;b&gt;PROJ.4 Cartographic Projections Library&lt;/b&gt; by Frank Warmerdam.</translation>
    </message>
    <message>
        <source>Developers in alphabetical order:&lt;br/&gt;Peter Curtis&lt;br/&gt;Kai Pastor&lt;br/&gt;Russell Porter&lt;br/&gt;Thomas Sch&amp;ouml;ps (project leader)&lt;br/&gt;&lt;br/&gt;For contributions, thanks to:&lt;br/&gt;Jon Cundill&lt;br/&gt;Jan Dalheimer&lt;br/&gt;Eugeniy Fedirets&lt;br/&gt;Peter Hoban&lt;br/&gt;Henrik Johansson&lt;br/&gt;Tojo Masaya&lt;br/&gt;Aivars Zogla&lt;br/&gt;&lt;br/&gt;Additional information:</source>
        <translation type="obsolete">Developers in alphabetical order:&lt;br/&gt;Peter Curtis&lt;br/&gt;Kai Pastor&lt;br/&gt;Russell Porter&lt;br/&gt;Thomas Sch&amp;ouml;ps (project leader)&lt;br/&gt;&lt;br/&gt;For contributions, thanks to:&lt;br/&gt;Jon Cundill&lt;br/&gt;Jan Dalheimer&lt;br/&gt;Eugeniy Fedirets&lt;br/&gt;Peter Hoban&lt;br/&gt;Henrik Johansson&lt;br/&gt;Tojo Masaya&lt;br/&gt;Aivars Zogla&lt;br/&gt;&lt;br/&gt;Additional information:</translation>
    </message>
    <message>
        <source>Failed to locate the help files.</source>
        <translation type="obsolete">ヘルプファイルを探すのに失敗しました。</translation>
    </message>
    <message>
        <source>Failed to start the help browser.</source>
        <translation type="obsolete">ヘルプブラウザの開始に失敗しました。</translation>
    </message>
    <message>
        <source>About %1</source>
        <translation type="obsolete">%1 について</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="obsolete">OK</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MainWindowController</name>
    <message>
        <location filename="../src/gui/main_window_controller.cpp" line="63"/>
        <location filename="../src/gui/main_window_controller.cpp" line="71"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window_controller.cpp" line="64"/>
        <source>Cannot export the map as
&quot;%1&quot;
because the format is unknown.</source>
        <translation>次の地図がエクスポートできません
&quot;%1&quot;
不明なフォーマットです。</translation>
    </message>
    <message>
        <location filename="../src/gui/main_window_controller.cpp" line="72"/>
        <source>Cannot export the map as
&quot;%1&quot;
because saving as %2 (.%3) is not supported.</source>
        <translation>次の地図がエクスポートできません。
&quot;%1&quot;
%2 (.%3) の保存はサポートされていません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Map</name>
    <message>
        <source>default layer</source>
        <translation type="obsolete">規定のレイヤー</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="200"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="253"/>
        <location filename="../src/templates/template_tool_paint.cpp" line="529"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <source>Cannot export the map as
&quot;%1&quot;
because saving as %2 (.%3) is not supported.</source>
        <translation type="vanished">次の地図が作成できません。
&quot;%1&quot;
%2 (%3)はサポートされていません。</translation>
    </message>
    <message>
        <source>Cannot export the map as
&quot;%1&quot;
because the format is unknown.</source>
        <translation type="vanished">次の地図が作成できません。
&quot;%1&quot;
不明なフォーマットです。</translation>
    </message>
    <message>
        <source>File does not exist or insufficient permissions to open:
%1</source>
        <translation type="vanished">ファイルが存在しないか、読み取り権限がありません:
%1</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation type="vanished">警告</translation>
    </message>
    <message>
        <source>The map export generated warnings.</source>
        <translation type="vanished">地図のエクスポート時に警告が発生しました。</translation>
    </message>
    <message>
        <source>Internal error while saving:
%1</source>
        <translation type="vanished">保存の際に内部エラーが発生しました:
%1</translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="572"/>
        <source>Cannot save file
%1:
%2</source>
        <translation>ファイルを保存できません
%1:
%2</translation>
    </message>
    <message>
        <source>The map import generated warnings.</source>
        <translation type="vanished">地図のインポート時に警告が発生しました。</translation>
    </message>
    <message>
        <source>Cannot open file:
%1
for reading.</source>
        <translation type="vanished">ファイルを開くことができません:
%1
を読み込むことができません。</translation>
    </message>
    <message>
        <source>Invalid file type.</source>
        <translation type="vanished">無効なファイル形式です。</translation>
    </message>
    <message>
        <source>Cannot open file:
%1

%2</source>
        <translation type="vanished">ファイルを開くことができません:
%1

%2</translation>
    </message>
    <message>
        <source>Problem while opening file:
%1

Error during symbol post-processing.</source>
        <translation type="obsolete">ファイルを開く際に問題が発生しました:
%1

記号のポストプロセッシング時にエラーが発生しました。</translation>
    </message>
    <message>
        <source>Nothing to import.</source>
        <translation type="vanished">インポートするものがありません。</translation>
    </message>
    <message>
        <source>Question</source>
        <translation type="vanished">質問</translation>
    </message>
    <message>
        <source>The scale of the imported data is 1:%1 which is different from this map&apos;s scale of 1:%2.

Rescale the imported data?</source>
        <translation type="vanished">インポートされた地図の縮尺 1:%1 と、編集中の地図の縮尺 1:%2 が異なります。

インポートされた地図の縮尺を変更しますか?</translation>
    </message>
    <message>
        <location filename="../src/core/map.cpp" line="480"/>
        <source>default part</source>
        <translation>デフォルトの地図パート</translation>
    </message>
    <message>
        <location filename="../src/core/map_color.cpp" line="36"/>
        <location filename="../src/core/map_color.cpp" line="50"/>
        <source>New color</source>
        <translation>新しい色</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4181"/>
        <source>Import...</source>
        <translation>インポート...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4182"/>
        <source>Symbol replacement was canceled.
Import the data anyway?</source>
        <translation>記号の置換がキャンセルされました。
それでもデータをインポートしますか?</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapColor</name>
    <message>
        <location filename="../src/core/map_color.cpp" line="79"/>
        <source>Registration black (all printed colors)</source>
        <translation>レジストレーションブラック (すべて印刷色)</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapCoord</name>
    <message>
        <location filename="../src/core/map_coord.cpp" line="183"/>
        <source>Coordinates are out-of-bounds.</source>
        <translation>座標は範囲外です。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapEditorController</name>
    <message>
        <source>One or more templates could not be loaded. Use the Templates -&gt; Template setup window to resolve the issue(s) by clicking on the red template file name(s).</source>
        <translation type="obsolete">一つ以上のテンプレートを読み込むことができませんでした。メニューバーの「テンプレート」から「テンプレートウィンドウ」を開いて、赤く表示されているテンプレートのファイル名をクリックし、問題を解決してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="920"/>
        <source>Print...</source>
        <translation>印刷...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="922"/>
        <source>&amp;Image</source>
        <translation>画像(&amp;I)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="924"/>
        <source>&amp;PDF</source>
        <translation>PDF(&amp;P)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="937"/>
        <source>Undo</source>
        <translation>元に戻す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="937"/>
        <source>Undo the last step</source>
        <translation>一つ前の操作を元に戻す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="938"/>
        <source>Redo</source>
        <translation>やり直す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="938"/>
        <source>Redo the last step</source>
        <translation>一つ前の操作をやり直す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="939"/>
        <source>Cu&amp;t</source>
        <translation>切り取り(&amp;T)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="941"/>
        <source>C&amp;opy</source>
        <translation>コピー(&amp;C)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="943"/>
        <source>&amp;Paste</source>
        <translation>貼り付け(&amp;P)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="946"/>
        <source>Select all</source>
        <translation>すべて選択</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="947"/>
        <source>Select nothing</source>
        <translation>何も選択しない</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="948"/>
        <source>Invert selection</source>
        <translation>選択を反転</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="952"/>
        <source>Clear undo / redo history</source>
        <translation>元に戻す/やり直すの履歴をクリア</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="952"/>
        <source>Clear the undo / redo history to reduce map file size.</source>
        <translation>元に戻す/やり直すの履歴をクリアし、地図のファイルサイズを縮小します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="954"/>
        <source>Show grid</source>
        <translation>グリッドを表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="955"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1268"/>
        <source>Configure grid...</source>
        <translation>グリッド設定...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="956"/>
        <source>Pan</source>
        <translation>画面移動</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="957"/>
        <source>Move to my location</source>
        <translation>自分の場所に移動</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="959"/>
        <source>Zoom in</source>
        <translation>拡大</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="960"/>
        <source>Zoom out</source>
        <translation>縮小</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="962"/>
        <source>Toggle fullscreen mode</source>
        <translation>フルスクリーン</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="963"/>
        <source>Set custom zoom factor...</source>
        <translation>ズーム率を指定(x)...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="965"/>
        <source>Hatch areas</source>
        <translation>面のハッチ表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="966"/>
        <source>Baseline view</source>
        <translation>ベースライン表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="967"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="131"/>
        <source>Hide all templates</source>
        <translation>すべてのテンプレートを非表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="968"/>
        <source>Overprinting simulation</source>
        <translation>オーバープリントをシミュレート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="970"/>
        <source>Symbol window</source>
        <translation>記号ウィンドウ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="970"/>
        <source>Show/Hide the symbol window</source>
        <translation>記号ウィンドウを表示/隠す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="971"/>
        <source>Color window</source>
        <translation>色ウィンドウ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="971"/>
        <source>Show/Hide the color window</source>
        <translation>色ウィンドウを表示/隠す</translation>
    </message>
    <message>
        <source>Load symbols from...</source>
        <translation type="obsolete">記号を読み込み...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="973"/>
        <source>Replace the symbols with those from another map file</source>
        <translation>他の地図ファイルの記号と置換え</translation>
    </message>
    <message>
        <source>Load colors from...</source>
        <translation type="obsolete">色を読み込む...</translation>
    </message>
    <message>
        <source>Replace the colors with those from another map file</source>
        <translation type="obsolete">色を他の地図ファイルのものと置き換え</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="977"/>
        <source>Scale all symbols...</source>
        <translation>すべての記号のスケールを変更...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="977"/>
        <source>Scale the whole symbol set</source>
        <translation>記号セットすべての縮尺を変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="979"/>
        <source>Change map scale...</source>
        <translation>地図の縮尺の変更...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="979"/>
        <source>Change the map scale and adjust map objects and symbol sizes</source>
        <translation>地図の縮尺の変更と、オブジェクトサイズと記号サイズの調節</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="980"/>
        <source>Rotate map...</source>
        <translation>地図の回転...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="980"/>
        <source>Rotate the whole map</source>
        <translation>地図全体を回転します</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="981"/>
        <source>Map notes...</source>
        <translation>ノート...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="983"/>
        <source>Template setup window</source>
        <translation>テンプレートウィンドウ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="983"/>
        <source>Show/Hide the template window</source>
        <translation>テンプレートウィンドウを表示/隠す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="986"/>
        <source>Open template...</source>
        <translation>テンプレートを開く...</translation>
    </message>
    <message>
        <source>Edit projection parameters...</source>
        <translation type="obsolete">投影パラメータの編集...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="961"/>
        <source>Show whole map</source>
        <translation>地図全体を表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="973"/>
        <source>Replace symbol set...</source>
        <translation>記号セットを置換...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="987"/>
        <source>Reopen template...</source>
        <translation>テンプレートを開きなおす...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="989"/>
        <source>Tag editor</source>
        <translation>タグ エディター</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="989"/>
        <source>Show/Hide the tag editor window</source>
        <translation>タグ エディター ウィンドウの表示/非表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="991"/>
        <source>Edit objects</source>
        <translation>オブジェクトの編集</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="992"/>
        <source>Edit lines</source>
        <translation>ラインの編集</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="993"/>
        <source>Set point objects</source>
        <translation>ポイントオブジェクトを配置</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="994"/>
        <source>Draw paths</source>
        <translation>コースを描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="995"/>
        <source>Draw circles and ellipses</source>
        <translation>円と楕円を描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="997"/>
        <source>Draw free-handedly</source>
        <translation>フリーハンドで描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="998"/>
        <source>Fill bounded areas</source>
        <translation>閉領域の塗りつぶし</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="999"/>
        <source>Write text</source>
        <translation>テキストを挿入</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="945"/>
        <source>Delete</source>
        <translation>削除</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1000"/>
        <source>Duplicate</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1001"/>
        <source>Switch symbol</source>
        <translation>記号を置換</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1002"/>
        <source>Fill / Create border</source>
        <translation>塗りつぶし/縁取り</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1003"/>
        <source>Switch dash direction</source>
        <translation>記号の向きの反転</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1004"/>
        <source>Connect paths</source>
        <translation>パスを接続</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1006"/>
        <source>Cut object</source>
        <translation>オブジェクトの切り取り</translation>
    </message>
    <message>
        <source>Cut holes</source>
        <translation type="obsolete">穴あけ</translation>
    </message>
    <message>
        <source>Rotate object(s)</source>
        <translation type="vanished">オブジェクトの回転</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1020"/>
        <source>Measure lengths and areas</source>
        <translation>長さと面積の測定</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1023"/>
        <source>Cut away from area</source>
        <translation>エリアから切り取り</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1025"/>
        <source>Merge area holes</source>
        <translation>エリアの穴をマージ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1026"/>
        <source>Convert to curves</source>
        <translation>曲線に変換</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1027"/>
        <source>Simplify path</source>
        <translation>パスを単純化</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1028"/>
        <source>Cutout</source>
        <translation>切り抜き</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1029"/>
        <source>Cut away</source>
        <translation>切り取り</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1030"/>
        <source>Distribute points along path</source>
        <translation>パスに沿ってポイントを配布する</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1038"/>
        <source>Paint on template settings</source>
        <translation>テンプレート設定に書き込み</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1043"/>
        <source>Enable touch cursor</source>
        <translation>タッチ カーソルを有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1044"/>
        <source>Enable GPS display</source>
        <translation>GPS 表示を有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1046"/>
        <source>Enable GPS distance rings</source>
        <translation>GPS 距離リングを有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1048"/>
        <source>Set point object at GPS position</source>
        <translation>GPS の位置でポイント オブジェクトを設定</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1050"/>
        <source>Set temporary marker at GPS position</source>
        <translation>GPS の位置で一時的なマーカーを設定</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1052"/>
        <source>Create temporary path at GPS position</source>
        <translation>GPS の位置で一時的なパスを作成</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1054"/>
        <source>Clear temporary GPS markers</source>
        <translation>一時的な GPS マーカーをクリア</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1057"/>
        <source>Enable compass display</source>
        <translation>コンパスの表示を有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1058"/>
        <source>Align map with north</source>
        <translation>地図を北に合わせる</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1060"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3745"/>
        <source>Add new part...</source>
        <translation>新しい地図パートを追加...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1061"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3805"/>
        <source>Rename current part...</source>
        <translation>現在の地図パートの名前を変更...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1062"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3766"/>
        <source>Remove current part</source>
        <translation>現在の地図パートを削除</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1063"/>
        <source>Merge all parts</source>
        <translation>すべての地図パートをマージ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1065"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4055"/>
        <source>Import...</source>
        <translation>インポート...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1085"/>
        <source>Copy position</source>
        <translation>位置をコピー</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1085"/>
        <source>Copy position to clipboard.</source>
        <translation>位置をクリップボードにコピーします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1120"/>
        <location filename="../src/gui/widgets/color_list_widget.cpp" line="113"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="275"/>
        <source>&amp;Edit</source>
        <translation>編集(&amp;E)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1141"/>
        <source>&amp;View</source>
        <translation>表示(&amp;V)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1165"/>
        <source>Toolbars</source>
        <translation>ツールバー</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1172"/>
        <source>&amp;Tools</source>
        <translation>ツール(&amp;T)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1225"/>
        <source>Sy&amp;mbols</source>
        <translation>記号(&amp;M)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1207"/>
        <source>M&amp;ap</source>
        <translation>地図(&amp;M)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="573"/>
        <source>Editing in progress</source>
        <translation>編集の処理中</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="574"/>
        <source>The map is currently being edited. Please finish the edit operation before saving.</source>
        <translation>地図は現在編集中です。保存する前に編集操作を終了してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1237"/>
        <source>&amp;Templates</source>
        <translation>テンプレート(&amp;T)</translation>
    </message>
    <message>
        <source>&amp;GPS</source>
        <translation type="obsolete">GPS(&amp;G)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1293"/>
        <source>Drawing</source>
        <translation>描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1032"/>
        <source>Paint on template</source>
        <translation>テンプレート上に書き込み</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="978"/>
        <source>Georeferencing...</source>
        <translation>ジオリファレンシング...</translation>
    </message>
    <message>
        <source>Draw circles</source>
        <translation type="obsolete">円を描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="996"/>
        <source>Draw rectangles</source>
        <translation>矩形を描画</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1007"/>
        <source>Cut free form hole</source>
        <translation>自由な形で穴あけ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1008"/>
        <source>Cut round hole</source>
        <translation>円形で穴あけ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1009"/>
        <source>Cut rectangular hole</source>
        <translation>矩形の穴あけ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1018"/>
        <source>Rotate pattern</source>
        <translation>パターンの回転</translation>
    </message>
    <message>
        <source>Scale object(s)</source>
        <translation type="vanished">オブジェクトのスケール変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1021"/>
        <source>Unify areas</source>
        <translation>エリアの論理和</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1022"/>
        <source>Intersect areas</source>
        <translation>エリアの論理積</translation>
    </message>
    <message>
        <source>Area difference</source>
        <translation type="vanished">エリアの論理差</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1024"/>
        <source>Area XOr</source>
        <translation>エリアの排他的論理和</translation>
    </message>
    <message>
        <source>Import</source>
        <translation type="obsolete">インポート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1067"/>
        <source>Map coordinates</source>
        <translation>マップ座標</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1069"/>
        <source>Projected coordinates</source>
        <translation>投影座標</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1071"/>
        <source>Latitude/Longitude (Dec)</source>
        <translation>緯度/経度 (Dec)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1073"/>
        <source>Latitude/Longitude (DMS)</source>
        <translation>緯度/経度 (DMS)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1155"/>
        <source>Display coordinates as...</source>
        <translation>座標の表示方法...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1109"/>
        <source>&amp;Export as...</source>
        <translation>形式を指定してエクスポート(&amp;E)...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1010"/>
        <source>Cut hole</source>
        <translation>穴あけ</translation>
    </message>
    <message>
        <source>Dummy</source>
        <translation type="vanished">ダミー</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1261"/>
        <source>View</source>
        <translation>表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1311"/>
        <source>Select template...</source>
        <translation>テンプレートの選択...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1317"/>
        <source>Editing</source>
        <translation>編集</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1342"/>
        <source>Advanced editing</source>
        <translation>高度な編集</translation>
    </message>
    <message>
        <source>Print or Export</source>
        <translation type="obsolete">プリント/エクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="604"/>
        <location filename="../src/gui/map/map_editor.cpp" line="612"/>
        <location filename="../src/gui/map/map_editor.cpp" line="647"/>
        <location filename="../src/gui/map/map_editor.cpp" line="660"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1755"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1775"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1831"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1851"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1864"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3267"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3273"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3279"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3285"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3294"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4100"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4131"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4208"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1755"/>
        <source>Print / Export is not available in this program version!</source>
        <translation>印刷 / エクスポートはこのバージョンのプログラムで利用できません!</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1775"/>
        <source>No undo steps available.</source>
        <translation>元に戻すことのできる操作がありません。</translation>
    </message>
    <message>
        <source>Cut %1 object(s)</source>
        <extracomment>Past tense. Displayed when an Edit &gt; Cut operation is completed.</extracomment>
        <translation type="vanished">%1 個のオブジェクトを切り取りました</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1831"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1864"/>
        <source>An internal error occurred, sorry!</source>
        <translation>内部エラーが発生しました!</translation>
    </message>
    <message>
        <source>Copied %1 object(s)</source>
        <translation type="vanished">%1 個のオブジェクトをコピーしました</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1851"/>
        <source>There are no objects in clipboard which could be pasted!</source>
        <translation>貼り付けのできるオブジェクトがクリップボードにありません!</translation>
    </message>
    <message>
        <source>Pasted %1 object(s)</source>
        <translation type="vanished">%1 個のオブジェクトを貼り付けました</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1949"/>
        <source>Set custom zoom factor</source>
        <translation>ズーム率を指定</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1949"/>
        <source>Zoom factor:</source>
        <translation>ズーム:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2023"/>
        <source>Symbols</source>
        <translation>記号</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2037"/>
        <source>Colors</source>
        <translation>色</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2058"/>
        <source>Symbol set ID</source>
        <translation>記号セット ID</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2059"/>
        <source>Edit the symbol set ID:</source>
        <translation>記号セット ID を編集:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2086"/>
        <source>Scale all symbols</source>
        <translation>すべての記号のスケールを変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2086"/>
        <source>Scale to percentage:</source>
        <translation>パーセントでスケール変更:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2110"/>
        <source>Map notes</source>
        <translation>ノート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2115"/>
        <source>Cancel</source>
        <translation>キャンセル</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2116"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2156"/>
        <source>Templates</source>
        <translation>テンプレート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2218"/>
        <source>Tag Editor</source>
        <translation>タグ エディター</translation>
    </message>
    <message>
        <source>Tag Selector</source>
        <translation type="vanished">タグ セレクター</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2282"/>
        <source>No
symbol
selected</source>
        <extracomment>Keep it short. Should not be much longer per line than the longest word in the original.</extracomment>
        <translation>記号が
選択されて
いません</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2284"/>
        <source>Multiple
symbols
selected</source>
        <extracomment>Keep it short. Should not be much longer per line than the longest word in the original.</extracomment>
        <translation>複数の
記号を
選択</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2384"/>
        <source>Place point objects on the map.</source>
        <translation>地図上にポイントオブジェクトを配置します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2384"/>
        <source>Select a point symbol to be able to use this tool.</source>
        <translation>このツールを有効にするには、ポイント記号を選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2386"/>
        <source>Draw polygonal and curved lines.</source>
        <translation>折れ線と曲線を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2386"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2388"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2390"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2392"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2394"/>
        <source>Select a line, area or combined symbol to be able to use this tool.</source>
        <translation>このツールを有効にするには、ライン記号/エリア記号/組み合わせ記号を選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2388"/>
        <source>Draw circles and ellipses.</source>
        <translation>円と楕円を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2392"/>
        <source>Draw paths free-handedly.</source>
        <translation>フリーハンドでコースを描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2394"/>
        <source>Fill bounded areas.</source>
        <translation>閉じられたエリアを塗りつぶします。</translation>
    </message>
    <message>
        <source>Deletes the selected object(s).</source>
        <translation type="vanished">選択したオブジェクトを削除します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2497"/>
        <source>Set the direction of area fill patterns or point objects.</source>
        <translation>エリアフィルパターンまたはポイントオブジェクトの方向をセットします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2497"/>
        <source>Select an area object with rotatable fill pattern or a rotatable point object to activate this tool.</source>
        <translation>このツールを有効にするには、回転可能なフィルパターンをもったエリアオブジェクト、または回転可能なポイントオブジェクトを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2527"/>
        <source>Resulting symbol: %1 %2.</source>
        <translation>結果の記号: %1 %2。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2528"/>
        <source>Select at least two area or path objects activate this tool.</source>
        <translation>少なくとも 2 つのエリアまたはパスオブジェクトを選択して、このツールを有効にします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2530"/>
        <source>Unify overlapping objects.</source>
        <translation>重なり合ったオブジェクトを結合します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2532"/>
        <source>Remove all parts which are not overlaps with the first selected object.</source>
        <translation>最初に選択したオブジェクトと重ならない要素をすべて削除します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2534"/>
        <source>Remove overlapped parts of the first selected object.</source>
        <translation>最初に選択したオブジェクトと重なった要素を削除します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2536"/>
        <source>Remove all parts which overlap the first selected object.</source>
        <translation>最初に選択したオブジェクトと重なった要素をすべて削除します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2540"/>
        <source>Merge area holes together, or merge holes with the object boundary to cut out this part.</source>
        <translation>エリアの穴を一緒にマージ、またはオブジェクトの境界と穴をマージして、この要素を切り取ります。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2540"/>
        <source>Select one area object with holes to activate this tool.</source>
        <translation>穴のあるエリア オブジェクトを 1 つ選択して、このツールを有効にします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3294"/>
        <source>Merging holes failed.</source>
        <translation>穴のマージに失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3570"/>
        <source>Clear temporary markers</source>
        <translation>一時的なマーカーをクリア</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3570"/>
        <source>Are you sure you want to delete all temporary GPS markers? This cannot be undone.</source>
        <translation>一時的な GPS マーカーをすべて削除してもよろしいですか? これは元に戻すことができません。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3685"/>
        <source>Merge this part with</source>
        <translation>この地図パートをマージ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3692"/>
        <source>Move selected objects to</source>
        <translation>選択したオブジェクトを移動</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3746"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3806"/>
        <source>Enter the name of the map part:</source>
        <translation>地図パートの名前を入力:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3822"/>
        <source>Switched to map part &apos;%1&apos;.</source>
        <translation>地図パート &apos;%1&apos; に切り替えました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3850"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3882"/>
        <source>Merge map parts</source>
        <translation>地図パートをマージ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3851"/>
        <source>Do you want to move all objects from map part &quot;%1&quot; to &quot;%2&quot;, and to remove &quot;%1&quot;?</source>
        <translation>すべてのオブジェクトを地図パート &quot;%1&quot; から &quot;%2&quot; に移動して、&quot;%1&quot; を削除しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3883"/>
        <source>Do you want to move all objects to map part &quot;%1&quot;, and to remove all other map parts?</source>
        <translation>すべてのオブジェクトを地図パート &quot;%1&quot; に移動して、他の地図パートをすべて削除しますか?</translation>
    </message>
    <message>
        <source>Import %1, GPX, OSM or DXF file</source>
        <translation type="vanished">%1、GPX、OSM、DXFファイルをインポート</translation>
    </message>
    <message>
        <source>Cannot import the selected map file because it could not be loaded.</source>
        <translation type="vanished">選択された地図はインポートできません。このファイルを読み込むことができませんでした。</translation>
    </message>
    <message>
        <source>Draw circles.</source>
        <translation type="obsolete">円を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2390"/>
        <source>Draw rectangles.</source>
        <translation>矩形を描画します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2396"/>
        <source>Write text on the map.</source>
        <translation>地図上にテキストを挿入します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2396"/>
        <source>Select a text symbol to be able to use this tool.</source>
        <translation>このツールを有効にするには、テキスト記号を選択してください。</translation>
    </message>
    <message>
        <source>Duplicate the selected object(s).</source>
        <translation type="vanished">選択中のオブジェクトを複製します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2486"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2488"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2490"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2492"/>
        <source>Select at least one object to activate this tool.</source>
        <translation>このツールを有効にするには、一つ以上のオブジェクトを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="584"/>
        <source>Map saved</source>
        <translation>地図を保存しました</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="598"/>
        <source>Cannot export the map as
&quot;%1&quot;
because saving as %2 (.%3) is not supported.</source>
        <translation>次の地図がエクスポートできません。
&quot;%1&quot;
%2 (.%3) の保存はサポートされていません。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="610"/>
        <source>Cannot save file
%1:
%2</source>
        <translation>ファイルを保存できません
%1:
%2</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="619"/>
        <location filename="../src/gui/map/map_editor.cpp" line="667"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4136"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="620"/>
        <source>The map export generated warnings.</source>
        <translation>地図のエクスポート時に警告が発生しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="667"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4136"/>
        <source>The map import generated warnings.</source>
        <translation>地図のインポート時に警告が発生しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="873"/>
        <source>Ctrl+A</source>
        <translation>Ctrl+A</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="874"/>
        <source>Ctrl+Shift+A</source>
        <translation>Ctrl+Shift+A</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="875"/>
        <source>Ctrl+I</source>
        <translation>Ctrl+I</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="877"/>
        <source>G</source>
        <translation>G</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="890"/>
        <source>E</source>
        <translation>E</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="891"/>
        <source>L</source>
        <translation>L</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="892"/>
        <source>S</source>
        <translation>S</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="893"/>
        <source>P</source>
        <translation>P</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="894"/>
        <source>O</source>
        <translation>O</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="895"/>
        <source>Ctrl+R</source>
        <translation>Ctrl+R</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="896"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="897"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="899"/>
        <source>D</source>
        <translation>D</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="900"/>
        <source>Ctrl+G</source>
        <translation>Ctrl+G</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="901"/>
        <source>Ctrl+F</source>
        <translation>Ctrl+F</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="902"/>
        <source>Ctrl+D</source>
        <translation>Ctrl+D</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="903"/>
        <source>C</source>
        <translation>C</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="904"/>
        <source>R</source>
        <translation>R</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="905"/>
        <source>Z</source>
        <translation>Z</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="906"/>
        <source>K</source>
        <translation>K</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="907"/>
        <source>H</source>
        <translation>H</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="908"/>
        <source>M</source>
        <translation>M</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="909"/>
        <source>U</source>
        <translation>U</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="910"/>
        <source>N</source>
        <translation>N</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="911"/>
        <source>Ctrl+M</source>
        <translation>Ctrl+M</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="972"/>
        <source>Symbol set ID...</source>
        <translation>記号セット ID...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="972"/>
        <source>Edit the symbol set ID</source>
        <translation>記号セット ID を編集</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="974"/>
        <source>Load CRT file...</source>
        <translation>CRT ファイルを読み込む...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="974"/>
        <source>Assign new symbols by cross-reference table</source>
        <translation>相互参照テーブルで、新しい記号を割り当てる</translation>
    </message>
    <message>
        <source>Import %1 or GPX file</source>
        <translation type="vanished">%1 または GPX ファイルのインポート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4208"/>
        <source>Nothing to import.</source>
        <translation>インポートするものがありません。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4220"/>
        <source>Question</source>
        <translation>質問</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4221"/>
        <source>The scale of the imported data is 1:%1 which is different from this map&apos;s scale of 1:%2.

Rescale the imported data?</source>
        <translation>インポートされた地図の縮尺 1:%1 と、この地図の縮尺 1:%2 が異なります。

インポートされた地図の縮尺を変更しますか?</translation>
    </message>
    <message>
        <source>Tag Selection</source>
        <translation type="vanished">タグ選択</translation>
    </message>
    <message>
        <source>Show/Hide the tag selection window</source>
        <translation type="vanished">タグ選択ウィンドウの表示/非表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1279"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1284"/>
        <location filename="../src/gui/map/map_editor.cpp" line="1423"/>
        <location filename="../src/gui/map/map_editor.cpp" line="3678"/>
        <source>Map parts</source>
        <translation>地図パート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1376"/>
        <source>Select symbol</source>
        <translation>記号を選択</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1416"/>
        <source>Hide top bar</source>
        <translation>上部のバーを非表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1419"/>
        <source>Show top bar</source>
        <translation>上部のバーを表示</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2501"/>
        <source>Switch the direction of symbols on line objects.</source>
        <translation>ラインオブジェクト上の記号の向きを反転します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2501"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2503"/>
        <source>Select at least one line object to activate this tool.</source>
        <translation>このツールを有効にするには、一つ以上のラインオブジェクトを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2503"/>
        <source>Connect endpoints of paths which are close together.</source>
        <translation>近接しているコースの終点を接続します。</translation>
    </message>
    <message>
        <source>Cut the selected object(s) into smaller parts.</source>
        <translation type="vanished">選択中のオブジェクトを分割します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2507"/>
        <source>Select at least one line or area object to activate this tool.</source>
        <translation>このツールを有効にするには、一つ以上のラインオブジェクトまたはエリアオブジェクトを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2516"/>
        <source>Cut a hole into the selected area object.</source>
        <translation>選択中のエリアオブジェクトに穴をあけます。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2516"/>
        <source>Select a single area object to activate this tool.</source>
        <translation>このツールを有効にするには、エリアオブジェクトを一つ選択してください。</translation>
    </message>
    <message>
        <source>Rotate the selected object(s).</source>
        <translation type="vanished">選択中のオブジェクトを回転します。</translation>
    </message>
    <message>
        <source>Scale the selected object(s).</source>
        <translation type="vanished">選択中のオブジェクトのスケールを変更します。</translation>
    </message>
    <message>
        <source>Unify overlapping areas.</source>
        <translation type="vanished">重なりのあるエリアオブジェクトを結合します。</translation>
    </message>
    <message>
        <source>Select at least two area objects with the same symbol to activate this tool.</source>
        <translation type="vanished">このツールを有効にするには、同じ記号を使用した二つ以上の.オブジェクトを選択してください。</translation>
    </message>
    <message>
        <source>Intersect the first selected area object with all other selected overlapping areas.</source>
        <translation type="vanished">重なったエリアの共通部分だけを出現させます。</translation>
    </message>
    <message>
        <source>Subtract all other selected area objects from the first selected area object.</source>
        <translation type="vanished">重なったエリアを片方のオブジェクトから切り取ります。</translation>
    </message>
    <message>
        <source>Select at least two area objects to activate this tool.</source>
        <translation type="vanished">このツールを有効にするには、二つ以上のオブジェクトを選択してください。</translation>
    </message>
    <message>
        <source>Calculate nonoverlapping parts of areas.</source>
        <translation type="vanished">重なったエリアを取り除きます。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2509"/>
        <source>Turn paths made of straight segments into smooth bezier splines.</source>
        <translation>直線のパスをスムーズなベジエ曲線に変えます。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2509"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2511"/>
        <source>Select a path object to activate this tool.</source>
        <translation>このツールを有効にするには、パスオブジェクトを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2511"/>
        <source>Reduce the number of points in path objects while trying to retain their shape.</source>
        <translation>パスオブジェクト内のポイントを(パスの形が変わらない範囲で)減らします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2545"/>
        <source>Create a cutout of some objects or the whole map.</source>
        <translation>いくつかのオブジェクトまたは地図全体の切り抜きを作成します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2545"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2547"/>
        <source>Select a closed path object as cutout shape to activate this tool.</source>
        <translation>切り抜きの形状として閉じられたパスオブジェクトを選択して、このツールを有効にします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2547"/>
        <source>Cut away some objects or everything in a limited area.</source>
        <translation>限定したエリアのいくつかのオブジェクトまたはすべてを切り取ります。</translation>
    </message>
    <message>
        <source>Switches the symbol of the selected object(s) to the selected symbol.</source>
        <translation type="vanished">選択中のオブジェクトの記号を、選択中の記号に置き換えます。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2561"/>
        <location filename="../src/gui/map/map_editor.cpp" line="2563"/>
        <source>Select at least one object and a fitting, different symbol to activate this tool.</source>
        <translation>このツールを有効にするには、一つ以上のオブジェクトと、それと置き換え可能な別の記号を選択してください。</translation>
    </message>
    <message>
        <source>Fill the selected line(s) or create a border for the selected area(s).</source>
        <translation type="vanished">選択中のラインオブジェクトを塗りつぶし、または選択中のエリアオブジェクトを縁取りします。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2566"/>
        <source>Places evenly spaced point objects along an existing path object</source>
        <translation>既存のパスオブジェクトに沿って、均等に間隔をあけてポイントオブジェクトを配置</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2566"/>
        <source>Select at least one path object and a single point symbol to activate this tool.</source>
        <translation>少なくとも 1 つのパスオブジェクトと単一のポイント記号を選択して、このツールを有効にします。</translation>
    </message>
    <message>
        <source>%1 object(s) duplicated</source>
        <translation type="vanished">%1 個のオブジェクトが複製されました</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2892"/>
        <source>Object selection</source>
        <translation>オブジェクトの選択</translation>
    </message>
    <message>
        <source>No objects were selected because there are no objects with the selected symbol(s).</source>
        <translation type="vanished">オブジェクトは選択されませんでした。選択中の記号を使用したオブジェクトがありません。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3253"/>
        <source>Measure</source>
        <translation>測定</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3267"/>
        <source>Unification failed.</source>
        <translation>エリアの論理和が失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3273"/>
        <source>Intersection failed.</source>
        <translation>エリアの論理積が失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3279"/>
        <source>Difference failed.</source>
        <translation>エリアの論理差が失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3285"/>
        <source>XOr failed.</source>
        <translation>エリアの排他的論理和が失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="3767"/>
        <source>Do you want to remove map part &quot;%1&quot; and all its objects?</source>
        <translation>地図パート「%1」と、そのすべてのオブジェクトを削除しますか?</translation>
    </message>
    <message>
        <source>Paint free-handedly on a template</source>
        <translation type="vanished">テンプレート上にフリーハンドで書き込みをします</translation>
    </message>
    <message>
        <source>Paint free-handedly on a template. Create or load a template which can be drawn onto to activate this button</source>
        <translation type="vanished">テンプレート上にフリーハンドで書き込みをします。書き込み可能なテンプレートを読み込んで、このボタンを有効にしてください</translation>
    </message>
    <message>
        <source>Import DXF or GPX file</source>
        <translation type="obsolete">DXFファイル、GBXファイルをインポート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4058"/>
        <source>Importable files</source>
        <translation>インポート可能なファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1719"/>
        <location filename="../src/gui/map/map_editor.cpp" line="4058"/>
        <source>All files</source>
        <translation>すべてのファイル</translation>
    </message>
    <message>
        <source>Import OMAP, OCD, GPX, OSM or DXF file</source>
        <translation type="obsolete">OMAP, OCD, GPX, OSM or DXF ファイルをインポート</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="4100"/>
        <source>Cannot import the selected file because its file format is not supported.</source>
        <translation>ファイルをインポートできません。このファイル形式はサポートされていません。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1017"/>
        <source>Rotate objects</source>
        <translation>オブジェクトの回転</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1019"/>
        <source>Scale objects</source>
        <translation>オブジェクトの拡大縮小</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1466"/>
        <source>1x zoom</source>
        <translation>1 倍ズーム</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1470"/>
        <source>2x zoom</source>
        <translation>2 倍ズーム</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1714"/>
        <source>Export</source>
        <translation>エクスポート</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/map/map_editor.cpp" line="1789"/>
        <source>Cut %n object(s)</source>
        <extracomment>Past tense. Displayed when an Edit &gt; Cut operation is completed.</extracomment>
        <translation>
            <numerusform>%n オブジェクトを切り取りました</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/map/map_editor.cpp" line="1841"/>
        <source>Copied %n object(s)</source>
        <translation>
            <numerusform>%n オブジェクトをコピーしました</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/map/map_editor.cpp" line="1881"/>
        <source>Pasted %n object(s)</source>
        <translation>
            <numerusform>%n オブジェクトを貼り付けました</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2486"/>
        <source>Deletes the selected objects.</source>
        <translation>選択したオブジェクトを削除します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2488"/>
        <source>Duplicate the selected objects.</source>
        <translation>選択したオブジェクトを複製します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2490"/>
        <source>Rotate the selected objects.</source>
        <translation>選択したオブジェクトを回転します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2492"/>
        <source>Scale the selected objects.</source>
        <translation>選択したオブジェクトを拡大縮小します。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2507"/>
        <source>Cut the selected objects into smaller parts.</source>
        <translation>選択したオブジェクトを小さな要素に切り取ります。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2561"/>
        <source>Switches the symbol of the selected objects to the selected symbol.</source>
        <translation>選択したオブジェクトの記号を選択した記号に切り替えます。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2563"/>
        <source>Fill the selected lines or create a border for the selected areas.</source>
        <translation>選択した線の塗りつぶし、または選択した領域の枠線を作成します。</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/map/map_editor.cpp" line="2682"/>
        <source>Duplicated %n object(s)</source>
        <translation>
            <numerusform>%n オブジェクトを複製しました</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="2892"/>
        <source>No objects were selected because there are no objects with the selected symbols.</source>
        <translation>選択した記号を持つオブジェクトがないため、オブジェクトは選択されませんでした。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapEditorTool</name>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="619"/>
        <location filename="../src/tools/cutout_tool.cpp" line="156"/>
        <location filename="../src/tools/draw_path_tool.cpp" line="1214"/>
        <location filename="../src/tools/draw_circle_tool.cpp" line="317"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="772"/>
        <location filename="../src/tools/draw_freehand_tool.cpp" line="290"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Abort. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 中止します。 </translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1233"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="725"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="726"/>
        <location filename="../src/tools/edit_point_tool.cpp" line="768"/>
        <source>More: %1, %2</source>
        <translation>もっと表示: %1, %2</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1230"/>
        <location filename="../src/tools/edit_line_tool.cpp" line="537"/>
        <source>More: %1</source>
        <translation>もっと表示: %1</translation>
    </message>
    <message>
        <location filename="../src/tools/draw_path_tool.cpp" line="1236"/>
        <location filename="../src/tools/draw_rectangle_tool.cpp" line="724"/>
        <source>More: %1, %2, %3</source>
        <translation>もっと表示: %1, %2, %3</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapFindFeature</name>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="53"/>
        <source>&amp;Find...</source>
        <translation>検索(&amp;F)...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="61"/>
        <source>Find &amp;next</source>
        <translation>次を検索(&amp;n)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="92"/>
        <source>Find objects</source>
        <translation>オブジェクトを検索</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="99"/>
        <source>&amp;Find next</source>
        <translation>次を検索(&amp;F)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="102"/>
        <source>Find &amp;all</source>
        <translation>すべて検索(&amp;a)</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="105"/>
        <source>Query editor</source>
        <translation>クエリ エディター</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapPrinter</name>
    <message>
        <location filename="../src/core/map_printer.cpp" line="513"/>
        <source>- Map -</source>
        <translation>- 地図 -</translation>
    </message>
    <message>
        <location filename="../src/core/map_printer.cpp" line="1328"/>
        <source>Processing separations of page %1...</source>
        <translation>ページ %1 のセパレーションを処理中...</translation>
    </message>
    <message>
        <location filename="../src/core/map_printer.cpp" line="1329"/>
        <source>Processing page %1...</source>
        <translation>ページ %1 を処理中...</translation>
    </message>
    <message>
        <location filename="../src/core/map_printer.cpp" line="1379"/>
        <source>Canceled</source>
        <translation>キャンセルされました</translation>
    </message>
    <message>
        <location filename="../src/core/map_printer.cpp" line="1383"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/core/map_printer.cpp" line="1388"/>
        <source>Finished</source>
        <translation>終了しました</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapSymbolTranslation</name>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="288"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="73"/>
        <source>Text source:</source>
        <translation>テキストソース:</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="540"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="321"/>
        <source>Map (%1)</source>
        <translation>地図 (%1)</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="541"/>
        <location filename="../src/gui/color_dialog.cpp" line="556"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="322"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="335"/>
        <source>undefined language</source>
        <translation>未定義の言語</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="559"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="338"/>
        <source>Translation (%1)</source>
        <translation>翻訳 (%1)</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="611"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="212"/>
        <source>Before editing, the stored text will be replaced with the current translation. Do you want to continue?</source>
        <translation>編集する前に、格納されたテキストは現在の翻訳に置き換えられます。続行しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/color_dialog.cpp" line="618"/>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="219"/>
        <source>After modifying the stored text, the translation may no longer be found. Do you want to continue?</source>
        <translation>格納されたテキストを変更した後、翻訳が見られなくなるかもしれません。続行しますか?</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MapWidget</name>
    <message>
        <source>Zoom: %1x</source>
        <translation type="obsolete">ズーム: %1x</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="590"/>
        <source>%1x</source>
        <comment>Zoom factor</comment>
        <translation>%1x</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="612"/>
        <source>mm</source>
        <comment>millimeters</comment>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="628"/>
        <source>m</source>
        <comment>meters</comment>
        <translation>m</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="665"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="847"/>
        <source>Empty map!

Start by defining some colors:
Select Symbols -&gt; Color window to
open the color dialog and
define the colors there.</source>
        <translation>空白の地図です。

まず色を定義するところから始めましょう。
メニューバーの「記号」から「色ウィンドウ」を選択して
色ダイアログを開き、色を定義してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="849"/>
        <source>No symbols!

Now define some symbols:
Right-click in the symbol bar
and select &quot;New symbol&quot;
to create one.</source>
        <translation>記号がありません！

まずは記号を定義してみましょう。
記号ウィンドウを右クリックして「新しい記号」を選択し、
記号を作成してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="851"/>
        <source>Ready to draw!

Start drawing or load a base map.
To load a base map, click
Templates -&gt; Open template...</source>
        <translation>地図を描く準備ができました。

作図を開始、または下絵を読み込んでください。
下絵を読み込むには、メニューバーの「テンプレート」から
「テンプレートを開く...」を選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_widget.cpp" line="851"/>
        <source>Hint: Hold the middle mouse button to drag the map,
zoom using the mouse wheel, if available.</source>
        <translation>ヒント: マウスのホイールドラッグで地図を移動できます。
マウスホイールを回転でズームすることができます。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::MeasureWidget</name>
    <message>
        <source>No objects selected.</source>
        <translation type="obsolete">オブジェクトが選択されていません。</translation>
    </message>
    <message>
        <source>No objects selected.&lt;br/&gt;Draw or select a path&lt;br/&gt;to measure it.</source>
        <translation type="obsolete">オブジェクトが選択されていません。&lt;br/&gt;測定する path を描画または選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="98"/>
        <source>Boundary length:</source>
        <translation>輪郭線の長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="136"/>
        <source>Length:</source>
        <translation>長さ:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="114"/>
        <source>Area:</source>
        <translation>面積:</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="61"/>
        <source>No object selected.</source>
        <translation>オブジェクトが選択されていません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="65"/>
        <source>%1 objects selected.</source>
        <translation>%1 オブジェクトが選択されています。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="75"/>
        <source>The selected object is not a path.</source>
        <translation>選択中のオブジェクトがパスではありません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="99"/>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="137"/>
        <source>mm</source>
        <comment>millimeters</comment>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="100"/>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="138"/>
        <source>m</source>
        <comment>meters</comment>
        <translation>m</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="115"/>
        <source>mm²</source>
        <comment>square millimeters</comment>
        <translation>mm²</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="116"/>
        <source>m²</source>
        <comment>square meters</comment>
        <translation>m²</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="128"/>
        <source>This object is too small.</source>
        <translation>オブジェクトが小さすぎます。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="129"/>
        <source>The minimimum area is %1 %2.</source>
        <translation>最小の面積は %1 %2 です。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="129"/>
        <source>mm²</source>
        <translation>mm²</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="132"/>
        <source>Note: Boundary length and area are correct only if there are no self-intersections and holes are used as such.</source>
        <translation>注意: エリアオブジェクトの輪郭線の長さと面積は、交差や穴あけがあると正しく測定されません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="150"/>
        <source>This line is too short.</source>
        <translation>ラインが短すぎます。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="151"/>
        <source>The minimum length is %1 %2.</source>
        <translation>最小の長さは %1 %2 です。</translation>
    </message>
    <message>
        <source>Selected object is not a path.</source>
        <translation type="obsolete">選択中のオブジェクトが path ではありません。</translation>
    </message>
    <message>
        <source>Boundary length</source>
        <translation type="obsolete">境界線の長さ</translation>
    </message>
    <message>
        <source>Length</source>
        <translation type="obsolete">長さ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/measure_widget.cpp" line="151"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <source>meters</source>
        <translation type="obsolete">m</translation>
    </message>
    <message>
        <source>Area</source>
        <translation type="obsolete">面積</translation>
    </message>
    <message>
        <source>&lt;br/&gt;&lt;u&gt;Warning&lt;/u&gt;: area is only correct if&lt;br/&gt;there are no self-intersections and&lt;br/&gt;holes are used as such.</source>
        <translation type="obsolete">&lt;br/&gt;&lt;u&gt;注意&lt;/u&gt;:面積は、エリアオブジェクトに重なりや穴あけがあると正しく測定されません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::NewMapDialog</name>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="66"/>
        <source>Create new map</source>
        <translation>新しい地図を作成</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="70"/>
        <source>Choose the scale and symbol set for the new map.</source>
        <translation>新しい地図の縮尺と記号セットを選択してください。</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="77"/>
        <source>Scale:  1 : </source>
        <translation>縮尺:  1 : </translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="82"/>
        <source>Symbol sets:</source>
        <translation>記号セット:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="87"/>
        <source>Only show symbol sets matching the selected scale</source>
        <translation>選択中の縮尺に一致する記号セットのみ表示する</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="vanished">キャンセル</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="95"/>
        <source>Create</source>
        <translation>作成</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="171"/>
        <source>Empty symbol set</source>
        <translation>空白の記号セット</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="209"/>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="264"/>
        <source>Load symbol set from a file...</source>
        <translation>ファイルから記号セットを読み込む...</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="260"/>
        <source>All symbol set files</source>
        <translation>すべての記号セットファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/map/new_map_dialog.cpp" line="262"/>
        <source>All files</source>
        <translation>すべてのファイル</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OCAD8FileExport</name>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1580"/>
        <source>The map contains more than 256 colors which is not supported by ocd version 8.</source>
        <translation>地図に256以上の色が含まれています。ocd バージョン 8 ではサポートされません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1584"/>
        <source>libocad returned %1</source>
        <translation>libocad が %1 を返しました</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1629"/>
        <source>Registration black is exported as a regular color.</source>
        <translation>レジストレーションブラックは、標準色としてエクスポートされます。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1737"/>
        <source>Unable to export fill pattern shift for an area object</source>
        <translation>エリアオブジェクトの塗りつぶしパターンのシフトをエクスポートできません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1872"/>
        <source>Unable to save correct position of missing template: &quot;%1&quot;</source>
        <translation>見つからないテンプレートの正しい位置を保存できません: &quot;%1&quot;</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1912"/>
        <source>Unable to export template: file type of &quot;%1&quot; is not supported yet</source>
        <translation>&quot;%1&quot; 形式のテンプレートのエクスポートは、まだサポートされていません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1936"/>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1947"/>
        <source>Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m).</source>
        <translation>座標は、OCAD 8 描画領域に収まるように調整されます (-2 m ... 2 m)。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1958"/>
        <source>Some coordinates remain outside of the OCAD 8 drawing area. They might be unreachable in OCAD.</source>
        <translation>一部の座標が OCAD 8 描画領域外に残っています。 OCAD で到達できない可能性があります。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2229"/>
        <source>In line symbol &quot;%1&quot;, cannot represent cap/join combination.</source>
        <translation>線の記号「%1」で、キャップ/結合の組み合わせを表すことはできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2250"/>
        <source>In line symbol &quot;%1&quot;, neglecting the dash grouping.</source>
        <translation>線の記号「%1」で、ダッシュ グループを無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2261"/>
        <source>In line symbol &quot;%1&quot;, the number of dashes in a group has been reduced to 2.</source>
        <translation>線の記号「%1」で、グループの破線の数を 2 に減らしました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2317"/>
        <source>In line symbol &quot;%1&quot;, cannot export the borders correctly.</source>
        <translation>線の記号「%1」で、境界線を正しくエクスポートできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2376"/>
        <source>In area symbol &quot;%1&quot;, skipping a fill pattern.</source>
        <translation>エリア記号「%1」で、塗りつぶしパターンをスキップします。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2423"/>
        <source>In area symbol &quot;%1&quot;, assuming a &quot;shifted rows&quot; point pattern. This might be correct as well as incorrect.</source>
        <translation>エリア記号「%1」に「シフト行」ポイントパターンを想定します。これは正しくない可能性もあります。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2462"/>
        <source>In text symbol %1: custom character spacing is set, its implementation does not match OCAD&apos;s behavior yet</source>
        <translation>テキスト記号 %1 で: カスタムの文字間隔を設定しました。その実装では OCAD の動作はまだ一致しません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2469"/>
        <source>In text symbol %1: ignoring underlining</source>
        <translation>テキスト記号 %1 で: 下線を無視しています</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2471"/>
        <source>In text symbol %1: ignoring kerning</source>
        <translation>テキスト記号 %1 で: カーニングを無視しています</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="2895"/>
        <source>String truncated (truncation marked with three &apos;|&apos;): %1</source>
        <translation>文字列を切り捨てました (切り捨てマークは 3 つの &apos;|&apos;): %1</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OCAD8FileImport</name>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="132"/>
        <source>Could not allocate buffer.</source>
        <translation>バッファーを割り当てできませんでした。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="136"/>
        <source>libocad returned %1</source>
        <translation>libocad が %1 を返しました</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="139"/>
        <source>OCAD files of version %1 are not supported!</source>
        <translation>バージョン %1 の OCAD ファイルはサポートされていません!</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="165"/>
        <source>%n color separation(s) were skipped, reason: Import disabled.</source>
        <translation>
            <numerusform>%n 色セパレーションをスキップしました。理由: インポートが無効です。</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="170"/>
        <source>Could not load the spot color definitions, error: %1</source>
        <translation>スポットカラーの定義を読み込むことができません。エラー: %1</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="231"/>
        <source>Color &quot;Registration black&quot; is imported as a special color.</source>
        <translation>色「レジストレーションブラック」は、特別色としてインポートされます。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="295"/>
        <source>Unable to import symbol &quot;%3&quot; (%1.%2)</source>
        <translation>記号 &quot;%3&quot; をインポートできません (%1.%2)</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="308"/>
        <source>OCAD import layer</source>
        <translation>OCADインポートレイヤー</translation>
    </message>
    <message>
        <source>In dashed line symbol %1, pointed cap lengths for begin and end are different (%2 and %3). Using %4.</source>
        <translation type="vanished">破線のライン記号 %1 で、三角のキャップの長さが、始点と終点で異なっています (%2 と %3)。%4 を使用してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="468"/>
        <source>In dashed line symbol %1, the end length cannot be imported correctly.</source>
        <translation>破線のライン記号 %1 で、末端の長さを正しくインポートできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="470"/>
        <source>In dashed line symbol %1, the end gap cannot be imported correctly.</source>
        <translation>破線のライン記号 %1 で、末端のギャップを正しくインポートできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="479"/>
        <source>In dashed line symbol %1, main and end length are different (%2 and %3). Using %4.</source>
        <translation>破線のライン記号 %1 で、本体と末端の長さが異なっています (%2 と %3)。%4 を使用してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="489"/>
        <source>In dashed line symbol %1, gaps D and E are different (%2 and %3). Using %4.</source>
        <translation>破線のライン記号 %1 で、キャップ D と E が異なっています (%2 と %3)。%4 を使用してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="639"/>
        <source>Line symbol %1: suppressing dash symbol at line ends.</source>
        <translation>線記号 %1: 線の終点のダッシュ記号を抑制します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="777"/>
        <source>During import of text symbol %1: ignoring justified alignment</source>
        <translation>テキスト記号 %1 をインポートする際、テキストの均等割付を無視します</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="783"/>
        <source>During import of text symbol %1: ignoring custom weight (%2)</source>
        <translation>テキスト記号 %1 をインポートする際、文字の太さ (weight) を無視します (%2)</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="788"/>
        <source>During import of text symbol %1: custom character spacing is set, its implementation does not match OCAD&apos;s behavior yet</source>
        <translation>テキスト記号 %1 をインポートする際、カスタムの文字間隔が設定されていますが、この実装はまだ OCAD の動作と一致しません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="793"/>
        <source>During import of text symbol %1: ignoring custom word spacing (%2%)</source>
        <translation>テキスト記号 %1 をインポートする際、カスタムの語間隔 (%2%) を無視します</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="798"/>
        <source>During import of text symbol %1: ignoring custom indents (%2/%3)</source>
        <translation>テキスト記号 %1 をインポートする際、カスタムの字下げ (%2/%3) を無視します</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="819"/>
        <source>During import of text symbol %1: ignoring text framing (mode %2)</source>
        <translation>テキスト記号 %1 をインポートする際、テキストフレーム (モード %2) を無視します</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="973"/>
        <source>Unable to load object</source>
        <translation>オブジェクトを読み込むことができません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="980"/>
        <source>Unable to import rectangle object</source>
        <translation>矩形オブジェクトを読み込むことができません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1027"/>
        <source>Not importing text symbol, couldn&apos;t figure out path&apos; (npts=%1): %2</source>
        <translation>テキスト記号をインポートしません。パスを把握できませんでした (npts=%1): %2</translation>
    </message>
    <message>
        <source>Unable to import template: %1</source>
        <translation type="obsolete">テンプレートをインポートすることができません: %1</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1219"/>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1328"/>
        <source>Unable to import template: background &quot;%1&quot; doesn&apos;t seem to be a raster image</source>
        <translation>テンプレートをインポートできません: 背景「%1」はラスター画像ではないようです</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1433"/>
        <source>Trying to import a text object with unknown coordinate format</source>
        <translation>不明な座標形式でテキスト オブジェクトをインポートしようとしています</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocad8_file_format.cpp" line="1544"/>
        <source>Color id not found: %1, ignoring this color</source>
        <translation>色 ID が見つかりませんでした %1 。この色を無視します</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ObjectQuery</name>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="295"/>
        <source>is</source>
        <extracomment>Very short label</extracomment>
        <translation>＝</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="298"/>
        <source>is not</source>
        <extracomment>Very short label</extracomment>
        <translation>≠</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="301"/>
        <source>contains</source>
        <extracomment>Very short label</extracomment>
        <translation>含む</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="304"/>
        <source>Search</source>
        <extracomment>Very short label</extracomment>
        <translation>検索</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="307"/>
        <source>Text</source>
        <extracomment>Very short label</extracomment>
        <translation>テキスト</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="311"/>
        <source>and</source>
        <extracomment>Very short label</extracomment>
        <translation>かつ</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="314"/>
        <source>or</source>
        <extracomment>Very short label</extracomment>
        <translation>または</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="318"/>
        <source>Symbol</source>
        <extracomment>Very short label</extracomment>
        <translation>記号</translation>
    </message>
    <message>
        <location filename="../src/core/objects/object_query.cpp" line="322"/>
        <source>invalid</source>
        <extracomment>Very short label</extracomment>
        <translation>無効</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdAreaSymbolCommon</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1352"/>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1387"/>
        <source>In area symbol &quot;%1&quot;, skipping a fill pattern.</source>
        <translation>エリア記号「%1」で、塗りつぶしパターンをスキップします。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1377"/>
        <source>In area symbol &quot;%1&quot;, assuming a &quot;shifted rows&quot; point pattern. This might be correct as well as incorrect.</source>
        <translation>エリア記号「%1」に「シフト行」ポイントパターンを想定します。これは正しくない可能性もあります。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdFileExport</name>
    <message>
        <location filename="../src/fileformats/ocd_georef_fields.cpp" line="879"/>
        <source>Could not translate coordinate reference system &apos;%1:%2&apos;.</source>
        <translation>座標参照系 &apos;%1:%2&apos; を変換できませんでした。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdFileImport</name>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="251"/>
        <source>In line symbol %1 &apos;%2&apos;: %3</source>
        <translation>線の記号 %1 &apos;%2&apos; で: %3</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="257"/>
        <source>In text symbol %1 &apos;%2&apos;: %3</source>
        <translation>テキスト記号 %1 &apos;%2&apos; で: %3</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_georef_fields.cpp" line="726"/>
        <source>Could not load the coordinate reference system &apos;%1&apos;.</source>
        <translation>座標参照系 &apos;%1&apos; を読み込みできませんでした。</translation>
    </message>
    <message>
        <source>Spot color information was ignored.</source>
        <translation type="vanished">スポット カラー情報は無視されました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="811"/>
        <source>Unable to import symbol %1.%2 &quot;%3&quot;: %4</source>
        <translation>記号 %1.%2 &quot;%3&quot; をインポートできません: %4</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="815"/>
        <source>Unsupported type &quot;%1&quot;.</source>
        <translation>サポートされていない種類 &quot;%1&quot;。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="913"/>
        <source>Unable to import template: &quot;%1&quot; is not a supported template type.</source>
        <translation>テンプレートをインポートすることができません: &quot;%1&quot; はサポートされているテンプレートの種類ではありません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1231"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1258"/>
        <source>Unsupported line style &apos;%1&apos;.</source>
        <translation>サポートされていない線の修飾 &apos;%1&apos;。</translation>
    </message>
    <message>
        <source>Different lengths for pointed caps at begin (%1 mm) and end (%2 mm) are not supported. Using %3 mm.</source>
        <translation type="vanished">始点 (%1 mm) と終点 (%2 mm) で異なる長さの三角のキャップはサポートされていません。%3 mm を使用しています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1304"/>
        <source>The dash pattern cannot be imported correctly.</source>
        <translation>破線パターンを正しくインポートできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1319"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1351"/>
        <source>The dash pattern&apos;s end length (%1 mm) cannot be imported correctly. Using %2 mm.</source>
        <translation>破線パターンの終点の長さ (%1 mm) を正しくインポートできません。%2 mm を使用しています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1326"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1366"/>
        <source>The dash pattern&apos;s end gap (%1 mm) cannot be imported correctly. Using %2 mm.</source>
        <translation>破線パターンの終点のギャップ (%1 mm) を正しくインポートできません。%2 mm を使用しています。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1405"/>
        <source>Unsupported framing line style &apos;%1&apos;.</source>
        <translation>サポートされていないフレームの線の修飾 &apos;%1&apos; です。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1476"/>
        <source>Skipped secondary point symbol.</source>
        <translation>セカンダリ ポイント記号をスキップしました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1521"/>
        <source>Suppressing dash symbol at line ends.</source>
        <translation>線の終点のダッシュ記号を抑制します。</translation>
    </message>
    <message>
        <source>This symbol cannot be saved as a proper OCD symbol again.</source>
        <translation type="vanished">この記号は、再度適切な OCD 記号として保存できません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2297"/>
        <source>Justified alignment is not supported.</source>
        <translation>両端揃えはサポートされていません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2312"/>
        <source>Vertical alignment &apos;%1&apos; is not supported.</source>
        <translation>縦方向の割り付け &apos;%1&apos; はサポートされていません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2284"/>
        <source>Ignoring custom weight (%1).</source>
        <translation>カスタム ウェイト (%1) を無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2321"/>
        <source>Custom character spacing may be incorrect.</source>
        <translation>カスタムの文字間隔が正しくない可能性があります。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2326"/>
        <source>Ignoring custom word spacing (%1 %).</source>
        <translation>カスタムの単語間隔 (%1 %) を無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2350"/>
        <source>Ignoring custom indents (%1/%2).</source>
        <translation>カスタムの字下げ (%1/%2) を無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2372"/>
        <source>Ignoring text framing (mode %1).</source>
        <translation>テキスト フレーム (モード %1) を無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1705"/>
        <source>Line text symbols are not yet supported. Marking the symbol as hidden.</source>
        <translation>ラインテキスト記号はまだサポートされていません。非表示として記号をマークします。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="112"/>
        <source>Encoding &apos;%1&apos; is not available. Check the settings.</source>
        <translation>エンコード &apos;%1&apos; は使用できません。設定を確認してください。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="245"/>
        <source>In area symbol %1 &apos;%2&apos;: %3</source>
        <translation>エリア記号 %1 &apos;%2&apos; で: %3</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="522"/>
        <source>Color &quot;%1&quot; is imported as special color &quot;Registration black&quot;.</source>
        <translation>色 &quot;%1&quot; は特別色 &quot;登録ブラック&quot; としてインポートされます。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1534"/>
        <source> - main line</source>
        <translation> - 主線</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1541"/>
        <source> - double line</source>
        <translation> - 二重線</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1548"/>
        <source> - framing</source>
        <translation> - フレーミング</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1586"/>
        <source>The border of this symbol could not be loaded.</source>
        <translation>この記号の境界を読み込みできませんでした。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1902"/>
        <source>Unable to load object</source>
        <translation>オブジェクトを読み込むことができません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1912"/>
        <source>Unable to import rectangle object</source>
        <translation>矩形オブジェクトを読み込むことができません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="1953"/>
        <source>Not importing text symbol, couldn&apos;t figure out path&apos; (npts=%1): %2</source>
        <translation>テキスト記号をインポートしません。パスを把握できませんでした (npts=%1): %2</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2259"/>
        <source>Trying to import a text object with unknown coordinate format</source>
        <translation>不明な座標形式でテキスト オブジェクトをインポートしようとしています</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2386"/>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2390"/>
        <source>Invalid data.</source>
        <translation>無効なデータ。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2417"/>
        <source>Support for OCD version %1 files is experimental.</source>
        <translation>OCD バージョン %1 ファイルのサポートは実験的です。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="2423"/>
        <source>OCD files of version %1 are not supported!</source>
        <translation>バージョン %1 の OCD ファイルはサポートされていません!</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_import.cpp" line="235"/>
        <source>Color id not found: %1, ignoring this color</source>
        <translation>色 ID が見つかりませんでした: %1。この色を無視します</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdLineSymbol</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="2147"/>
        <source>In line symbol &quot;%1&quot;, cannot represent cap/join combination.</source>
        <translation>線の記号「%1」で、キャップ/結合の組み合わせを表すことはできません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdLineSymbolCommon</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1509"/>
        <source>In line symbol &quot;%1&quot;, cannot represent cap/join combination.</source>
        <translation>線の記号「%1」で、キャップ/結合の組み合わせを表すことはできません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1530"/>
        <source>In line symbol &quot;%1&quot;, neglecting the dash grouping.</source>
        <translation>線の記号「%1」で、破線グループを無視します。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1541"/>
        <source>In line symbol &quot;%1&quot;, the number of dashes in a group has been reduced to 2.</source>
        <translation>線の記号「%1」で、グループの破線の数を 2 に減らしました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1635"/>
        <source>In line symbol &quot;%1&quot;, cannot export the borders correctly.</source>
        <translation>線の記号「%1」で、境界線を正しくエクスポートできません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OcdTextSymbolBasic</name>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1826"/>
        <source>In text symbol %1: custom character spacing is set,its implementation does not match OCAD&apos;s behavior yet</source>
        <translation>テキスト記号 %1: カスタム文字間隔が設定され、その実装はまだ OCAD の動作と一致しません</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1842"/>
        <source>In text symbol %1: ignoring underlining</source>
        <translation>テキスト記号 %1 で: 下線を無視しています</translation>
    </message>
    <message>
        <location filename="../src/fileformats/ocd_file_export.cpp" line="1844"/>
        <source>In text symbol %1: ignoring kerning</source>
        <translation>テキスト記号 %1 で: カーニングを無視しています</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OgrFileExport</name>
    <message>
        <source>Unknown file extension %1, only GPX, KML, and SHP files are supported.</source>
        <translation type="vanished">不明なファイル拡張子 %1。GPX、KML、および SHP ファイルのみがサポートされています。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1679"/>
        <source>Couldn&apos;t find a driver for file extension %1</source>
        <translation>ファイル拡張子 %1 のドライバーが見つかりませんでした</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1691"/>
        <source>Failed to create dataset: %1</source>
        <translation>データセットの作成に失敗しました: %1</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1736"/>
        <source>Failed to create layer: %2</source>
        <translation>レイヤーの作成に失敗しました: %2</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1874"/>
        <source>The map is not georeferenced. Local georeferencing only.</source>
        <translation>マップはジオリファレンスされません。ローカルジオリファレンスのみ。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1887"/>
        <source>Failed to properly export the georeferencing info. Local georeferencing only.</source>
        <translation>ジオリファレンス情報を正しくエクスポートできませんでした。ローカルジオリファレンスのみ。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1897"/>
        <source>The %1 driver requires valid georefencing info.</source>
        <translation>%1 ドライバーには、有効なジオリフェンシング情報が必要です。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="1972"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="2021"/>
        <source>Failed to create feature in layer: %1</source>
        <translation>レイヤーにフィーチャーを作成できませんでした: %1</translation>
    </message>
    <message>
        <source>Failed to create layer %1: %2</source>
        <translation type="vanished">レイヤー %1 の作成に失敗しました: %2</translation>
    </message>
    <message>
        <source>Failed to create name field: %1</source>
        <translation type="vanished">名前フィールドの作成に失敗しました: %1</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::OgrFileImport</name>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="631"/>
        <source>Unable to setup &quot;%1&quot; SRS for GDAL: %2</source>
        <translation>GDAL の &quot;%1&quot; SRS をセットアップできません: %2</translation>
    </message>
    <message>
        <source>Purple</source>
        <translation type="vanished">紫</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="567"/>
        <source>Point</source>
        <translation>ポイント</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="574"/>
        <source>Line</source>
        <translation>線</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="583"/>
        <source>Area</source>
        <translation>エリア</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="589"/>
        <source>Text</source>
        <translation>テキスト</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="684"/>
        <source>Unable to load layer %1.</source>
        <translation>レイヤー %1 を読み込みできません。</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gdal/ogr_file_format.cpp" line="732"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="737"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="742"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="747"/>
        <location filename="../src/gdal/ogr_file_format.cpp" line="752"/>
        <source>Unable to load %n objects, reason: %1</source>
        <translation>
            <numerusform>%n オブジェクトを読み込みできません。 理由: %1</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="733"/>
        <source>Empty geometry.</source>
        <translation>空のジオメトリ。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="738"/>
        <source>Can&apos;t determine the coordinate transformation: %1</source>
        <translation>座標変換を決定できません: %1</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="743"/>
        <source>Failed to transform the coordinates.</source>
        <translation>座標の変換に失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="748"/>
        <source>Unknown or unsupported geometry type.</source>
        <translation>不明なまたはサポートされていないジオメトリタイプ。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="753"/>
        <source>Not enough coordinates.</source>
        <translation>座標が不足しています。</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="798"/>
        <source>Cannot use this spatial reference:
%1</source>
        <translation>この空間参照は使用できません:
%1</translation>
    </message>
    <message>
        <location filename="../src/gdal/ogr_file_format.cpp" line="860"/>
        <source>The geospatial data has no suitable spatial reference.</source>
        <translation>地理空間データに、適切な空間参照がありません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PaintOnTemplateSelectDialog</name>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="460"/>
        <source>Select template to draw onto</source>
        <translation>書き込むテンプレートを選択</translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="487"/>
        <source>Cancel</source>
        <translation>キャンセル</translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="488"/>
        <source>Draw</source>
        <translation>書き込む</translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="558"/>
        <source>Template file exists: &apos;%1&apos;</source>
        <translation>テンプレートファイルが存在します: &apos;%1&apos;</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PaintOnTemplateTool</name>
    <message>
        <source>&lt;b&gt;Left mouse click and drag&lt;/b&gt; to paint, &lt;b&gt;Right mouse click and drag&lt;/b&gt; to erase</source>
        <translation type="obsolete">&lt;b&gt;左クリックしながらドラッグ&lt;/b&gt;で書き込み、&lt;b&gt;右クリックしながらドラッグ&lt;/b&gt;で消去します</translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="161"/>
        <source>&lt;b&gt;Click and drag&lt;/b&gt;: Paint. &lt;b&gt;Right click and drag&lt;/b&gt;: Erase. </source>
        <translation>&lt;b&gt;左クリックしながらドラッグ&lt;/b&gt;: ペイントします。&lt;b&gt;右クリックしながらドラッグ&lt;/b&gt;: 消去します。 </translation>
    </message>
    <message>
        <location filename="../src/templates/template_tool_paint.cpp" line="164"/>
        <source>Color selection</source>
        <translation>色を選択</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PanTool</name>
    <message>
        <location filename="../src/tools/pan_tool.cpp" line="80"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the map. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 地図を移動します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PercentageDelegate</name>
    <message>
        <location filename="../src/util/item_delegates.cpp" line="155"/>
        <location filename="../src/util/item_delegates.cpp" line="162"/>
        <source>%</source>
        <translation>%</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PointSymbolEditorTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to add a coordinate, &lt;b&gt;Ctrl+Click&lt;/b&gt; to change the selected coordinate</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で座標を追加、&lt;b&gt;Ctrl キーを押しながらクリック&lt;/b&gt;で、選択中の座標を変更します</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="987"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Add a coordinate. &lt;b&gt;%1+Click&lt;/b&gt;: Change the selected coordinate. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 座標を追加します。&lt;b&gt;%1 + クリック&lt;/b&gt;: 選択中の座標を変更します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PointSymbolEditorWidget</name>
    <message>
        <source>Point editor</source>
        <translation type="obsolete">ポイントエディタ</translation>
    </message>
    <message>
        <source>Edit:</source>
        <translation type="obsolete">編集:</translation>
    </message>
    <message>
        <source>Elements:</source>
        <translation type="obsolete">要素:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="105"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="100"/>
        <source>Always oriented to north (not rotatable)</source>
        <translation>常に北に向けて正置 (回転禁止)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="108"/>
        <source>Elements</source>
        <translation>要素</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="120"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="946"/>
        <source>Point</source>
        <translation>ポイント</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="121"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="948"/>
        <source>Line</source>
        <translation>ライン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="122"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="950"/>
        <source>Area</source>
        <translation>エリア</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="125"/>
        <source>Center all elements</source>
        <translation>すべての要素を中央寄せ</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="127"/>
        <source>Current element</source>
        <translation>現在の要素</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="133"/>
        <source>Diameter &lt;b&gt;a&lt;/b&gt;:</source>
        <translation>直径 &lt;b&gt;a&lt;/b&gt;:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="134"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="140"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="168"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="136"/>
        <source>Inner color:</source>
        <translation>インナーの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="139"/>
        <source>Outer width &lt;b&gt;b&lt;/b&gt;:</source>
        <translation>アウター &lt;b&gt;b&lt;/b&gt;:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="142"/>
        <source>Outer color:</source>
        <translation>アウターの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="167"/>
        <source>Line width:</source>
        <translation>ラインの幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="170"/>
        <source>Line color:</source>
        <translation>ラインの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="173"/>
        <source>Line cap:</source>
        <translation>ラインのキャップ:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="175"/>
        <source>flat</source>
        <translation>平ら</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="176"/>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="183"/>
        <source>round</source>
        <translation>ラウンド結合</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="177"/>
        <source>square</source>
        <translation>突出線端</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="180"/>
        <source>Line join:</source>
        <translation>角の形状:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="182"/>
        <source>miter</source>
        <translation>マイター結合</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="184"/>
        <source>bevel</source>
        <translation>ベベル結合</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="186"/>
        <source>Line closed</source>
        <translation>ラインを閉じる</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="206"/>
        <source>Area color:</source>
        <translation>エリアの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="219"/>
        <source>Coordinates:</source>
        <translation>座標:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="224"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="224"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="224"/>
        <source>Curve start</source>
        <translation>曲線を開始</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="235"/>
        <source>Center by coordinate average</source>
        <translation>座標平均に中心を置く</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="419"/>
        <source>[Midpoint]</source>
        <translation>[中間点]</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_editor_widget.cpp" line="953"/>
        <source>Unknown</source>
        <translation>不明</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PointSymbolSettings</name>
    <message>
        <source>Point settings</source>
        <translation type="obsolete">ポイント記号の設定</translation>
    </message>
    <message>
        <source>Always oriented to north (not rotatable)</source>
        <translation type="obsolete">常に北に向けて正置 (回転禁止)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/point_symbol_settings.cpp" line="60"/>
        <source>Point symbol</source>
        <translation>ポイント記号</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PrintProgressDialog</name>
    <message>
        <location filename="../src/gui/print_progress_dialog.cpp" line="58"/>
        <source>Printing</source>
        <comment>PrintWidget</comment>
        <translation>印刷中</translation>
    </message>
    <message>
        <location filename="../src/gui/print_progress_dialog.cpp" line="59"/>
        <source>An error occurred during processing.</source>
        <comment>PrintWidget</comment>
        <translation>処理中にエラーが発生しました。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PrintTool</name>
    <message>
        <source>&lt;b&gt;Drag&lt;/b&gt; to move the print area</source>
        <translation type="obsolete">&lt;b&gt;ドラッグ&lt;/b&gt;で印刷領域を移動します</translation>
    </message>
    <message>
        <location filename="../src/gui/print_tool.cpp" line="59"/>
        <location filename="../src/gui/print_tool.cpp" line="371"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the map, the print area or the area&apos;s borders. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 地図、プリントエリア、エリアの縁を移動します。 </translation>
    </message>
    <message>
        <location filename="../src/gui/print_tool.cpp" line="343"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the print area. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: プリントエリアを移動します。 </translation>
    </message>
    <message>
        <location filename="../src/gui/print_tool.cpp" line="347"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the map. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 地図を移動します。 </translation>
    </message>
    <message>
        <location filename="../src/gui/print_tool.cpp" line="352"/>
        <location filename="../src/gui/print_tool.cpp" line="357"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the print area&apos;s border. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: プリントエリアの縁を移動します。 </translation>
    </message>
    <message>
        <location filename="../src/gui/print_tool.cpp" line="362"/>
        <location filename="../src/gui/print_tool.cpp" line="367"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move the print area&apos;s borders. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: プリントエリアの縁を移動します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::PrintWidget</name>
    <message>
        <source>Printer or exporter:</source>
        <translation type="obsolete">プリンタまたはエクスポータの選択:</translation>
    </message>
    <message>
        <source>Export to PDF or PS</source>
        <translation type="vanished">PDF または PSをエクスポート</translation>
    </message>
    <message>
        <source>Export to image</source>
        <translation type="obsolete">イメージをエクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="262"/>
        <source>Show templates</source>
        <translation>テンプレートを表示する</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="273"/>
        <source>Show grid</source>
        <translation>グリッドを表示</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="180"/>
        <source>Page orientation:</source>
        <translation>ページの方向:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="140"/>
        <source>Printer:</source>
        <translation>プリンター:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="173"/>
        <source>Portrait</source>
        <translation>縦</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="175"/>
        <source>Landscape</source>
        <translation>横</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="154"/>
        <source>Page format:</source>
        <translation>印刷のサイズ:</translation>
    </message>
    <message>
        <source>Dots per inch (dpi):</source>
        <translation type="obsolete">ドット/インチ(dpi):</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="183"/>
        <source>Copies:</source>
        <translation>印刷部数:</translation>
    </message>
    <message>
        <source>Print area</source>
        <translation type="obsolete">印刷領域</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="196"/>
        <source>Left:</source>
        <translation>左:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="199"/>
        <source>Top:</source>
        <translation>上:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="202"/>
        <source>Width:</source>
        <translation>幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="205"/>
        <source>Height:</source>
        <translation>高さ:</translation>
    </message>
    <message>
        <source>Center area on map</source>
        <translation type="obsolete">地図の中心領域を印刷</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="303"/>
        <source>Preview...</source>
        <translation>プレビュー...</translation>
    </message>
    <message>
        <source>Export</source>
        <translation type="obsolete">エクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="188"/>
        <source>Single page</source>
        <translation>単一ページ</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="189"/>
        <source>Custom area</source>
        <translation>カスタムエリア</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="190"/>
        <source>Map area:</source>
        <translation>マップエリア:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="192"/>
        <source>Center print area</source>
        <translation>地図の中心を印刷</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="160"/>
        <location filename="../src/gui/print_widget.cpp" line="164"/>
        <location filename="../src/gui/print_widget.cpp" line="195"/>
        <location filename="../src/gui/print_widget.cpp" line="198"/>
        <location filename="../src/gui/print_widget.cpp" line="201"/>
        <location filename="../src/gui/print_widget.cpp" line="204"/>
        <location filename="../src/gui/print_widget.cpp" line="207"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="208"/>
        <source>Page overlap:</source>
        <translation>ページのオーバーラップ:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="212"/>
        <source>Options</source>
        <translation>オプション</translation>
    </message>
    <message>
        <source>Normal output</source>
        <translation type="vanished">通常のアウトプット</translation>
    </message>
    <message>
        <source>Color separations</source>
        <translation type="vanished">色分解</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="246"/>
        <source>Resolution:</source>
        <translation>解像度:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="248"/>
        <source>Print in different scale:</source>
        <translation>縮尺を変更して印刷:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="269"/>
        <source>Template appearance may differ.</source>
        <translation>テンプレートの外観が異なる場合があります。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="276"/>
        <source>Simulate overprinting</source>
        <translation>オーバープリントをシミュレートする</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="238"/>
        <source>Default</source>
        <translation>デフォルト</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="239"/>
        <source>Device CMYK</source>
        <translation>デバイス CMYK</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="240"/>
        <source>Color mode:</source>
        <translation>カラーモード:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="305"/>
        <location filename="../src/gui/print_widget.cpp" line="411"/>
        <source>Print</source>
        <translation>印刷</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="309"/>
        <source>Export...</source>
        <translation>エクスポート...</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="418"/>
        <source>PDF export</source>
        <translation>PDFをエクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="431"/>
        <source>Image export</source>
        <translation>イメージをエクスポート</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="537"/>
        <source>Save to PDF</source>
        <translation>PDFに保存</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="244"/>
        <location filename="../src/gui/print_widget.cpp" line="942"/>
        <location filename="../src/gui/print_widget.cpp" line="992"/>
        <source>dpi</source>
        <translation>dpi</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="279"/>
        <source>Save world file</source>
        <translation>world ファイルを保存</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="959"/>
        <source>The map contains transparent elements which require the raster mode.</source>
        <translation>マップに、ラスターモードを必要とする透明な要素が含まれています。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1122"/>
        <source>Not supported on Android.</source>
        <translation>Android ではサポートされていません。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1130"/>
        <source>Failed to prepare the preview.</source>
        <translation>プレビューの準備に失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1141"/>
        <source>Print Preview Progress</source>
        <translation>印刷プレビューの進行状況</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1158"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1159"/>
        <source>A non-standard view mode is activated. Are you sure to print / export the map like this?</source>
        <translation>非標準の表示モードが有効です。地図をこのように印刷 / エクスポートしてもよろしいですか?</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1176"/>
        <source>PNG</source>
        <translation>PNG</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1177"/>
        <source>BMP</source>
        <translation>BMP</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1178"/>
        <source>TIFF</source>
        <translation>TIFF</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1179"/>
        <source>JPEG</source>
        <translation>JPEG</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1251"/>
        <source>Failed to prepare the PDF export.</source>
        <translation>PDF エクスポートの準備に失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1261"/>
        <source>PDF</source>
        <translation>PDF</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1314"/>
        <source>An error occurred during printing.</source>
        <translation>印刷中にエラーが発生しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1181"/>
        <location filename="../src/gui/print_widget.cpp" line="1211"/>
        <location filename="../src/gui/print_widget.cpp" line="1263"/>
        <location filename="../src/gui/print_widget.cpp" line="1275"/>
        <source>Export map ...</source>
        <translation>地図のエクスポート...</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1180"/>
        <location filename="../src/gui/print_widget.cpp" line="1262"/>
        <source>All files (*.*)</source>
        <translation>すべてのファイル (*.*)</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1300"/>
        <source>Failed to prepare the printing.</source>
        <translation>印刷の準備に失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1309"/>
        <source>Printing Progress</source>
        <translation>印刷の進行状況</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="958"/>
        <location filename="../src/gui/print_widget.cpp" line="1122"/>
        <location filename="../src/gui/print_widget.cpp" line="1130"/>
        <location filename="../src/gui/print_widget.cpp" line="1199"/>
        <location filename="../src/gui/print_widget.cpp" line="1220"/>
        <location filename="../src/gui/print_widget.cpp" line="1251"/>
        <location filename="../src/gui/print_widget.cpp" line="1281"/>
        <location filename="../src/gui/print_widget.cpp" line="1300"/>
        <location filename="../src/gui/print_widget.cpp" line="1314"/>
        <location filename="../src/gui/print_widget.cpp" line="1327"/>
        <location filename="../src/gui/print_widget.cpp" line="1375"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="145"/>
        <source>Properties</source>
        <translation>プロパティ</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="219"/>
        <source>Vector
graphics</source>
        <translation>ベクター
グラフィック</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="220"/>
        <source>Raster
graphics</source>
        <translation>ラスター
グラフィック</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="221"/>
        <source>Color
separations</source>
        <translation>色
分版</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="234"/>
        <source>Mode:</source>
        <translation>モード:</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1199"/>
        <source>Failed to prepare the image. Not enough memory.</source>
        <translation>画像の準備に失敗しました。メモリが不足しています。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1220"/>
        <source>Failed to save the image. Does the path exist? Do you have sufficient rights?</source>
        <translation>画像の保存に失敗しました。ファイルパスは存在しますか?　十分な権利はもっていますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1224"/>
        <location filename="../src/gui/print_widget.cpp" line="1285"/>
        <source>Exported successfully to %1</source>
        <translation>%1 へのエクスポートは成功しました</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1281"/>
        <source>Failed to finish the PDF export.</source>
        <translation>PDF エクスポートを完了できませんでした。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1291"/>
        <location filename="../src/gui/print_widget.cpp" line="1323"/>
        <source>Canceled.</source>
        <translation>キャンセルされました。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1327"/>
        <source>The print job could not be stopped.</source>
        <translation>プリントジョブを停止できません。</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1318"/>
        <source>Successfully created print job</source>
        <translation>プリントジョブの作成は成功しました</translation>
    </message>
    <message>
        <source>Unknown</source>
        <comment>Paper size</comment>
        <translation type="vanished">不明</translation>
    </message>
    <message>
        <location filename="../src/gui/print_widget.cpp" line="1375"/>
        <source>The map area is empty. Output canceled.</source>
        <translation>マップエリアが空白です。出力はキャンセルされました。</translation>
    </message>
    <message>
        <source>Letter</source>
        <translation type="obsolete">レター</translation>
    </message>
    <message>
        <source>Legal</source>
        <translation type="obsolete">リーガル</translation>
    </message>
    <message>
        <source>Executive</source>
        <translation type="obsolete">エグゼクティブ</translation>
    </message>
    <message>
        <source>C5E</source>
        <translation type="obsolete">C5E</translation>
    </message>
    <message>
        <source>Comm10E</source>
        <translation type="obsolete">US Comm. Env. #10 ［#10封筒］</translation>
    </message>
    <message>
        <source>DLE</source>
        <translation type="obsolete">EUR DL Env. ［DL封筒］</translation>
    </message>
    <message>
        <source>Folio</source>
        <translation type="obsolete">Folio</translation>
    </message>
    <message>
        <source>Ledger</source>
        <translation type="obsolete">Ledger</translation>
    </message>
    <message>
        <source>Tabloid</source>
        <translation type="obsolete">タブロイド</translation>
    </message>
    <message>
        <source>Custom</source>
        <translation type="obsolete">カスタム</translation>
    </message>
    <message>
        <source>Unknown</source>
        <translation type="obsolete">不明</translation>
    </message>
    <message>
        <source>The map is empty, there is nothing to print!</source>
        <translation type="obsolete">印刷する地図がありません!</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ProjectedCRSSelector</name>
    <message>
        <source>&amp;Coordinate reference system:</source>
        <translation type="vanished">座標参照系 (&amp;C) :</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ReopenTemplateDialog</name>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="41"/>
        <source>Reopen template</source>
        <translation>テンプレートを開きなおす</translation>
    </message>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="43"/>
        <source>Drag items from the left list to the desired spot in the right list to reload them.</source>
        <translation>アイテムを左のリストから右のリストの任意の場所にドラッグで再ロードします。</translation>
    </message>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="45"/>
        <source>Closed templates:</source>
        <translation>閉じたテンプレート:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="48"/>
        <source>Clear list</source>
        <translation>リストをクリア</translation>
    </message>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="51"/>
        <source>Active templates:</source>
        <translation>アクティブなテンプレート:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_dialog_reopen.cpp" line="60"/>
        <source>- Map -</source>
        <translation>- 地図 -</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::RotateMapDialog</name>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="46"/>
        <source>Rotate map</source>
        <translation>地図を回転</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="53"/>
        <source>Angle (counter-clockwise):</source>
        <translation>角度 (反時計回り):</translation>
    </message>
    <message>
        <source>°</source>
        <translation type="vanished">°</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="50"/>
        <source>Rotation parameters</source>
        <translation>回転パラメータ</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="55"/>
        <source>Rotate around:</source>
        <translation>回転の軸:</translation>
    </message>
    <message>
        <source>Map coordinate system origin</source>
        <comment>Rotation center point</comment>
        <translation type="vanished">地図座標系の原点</translation>
    </message>
    <message>
        <source>Georeferencing reference point</source>
        <comment>Rotation center point</comment>
        <translation type="vanished">ジオリファレンスの参照点</translation>
    </message>
    <message>
        <source>Other point,</source>
        <comment>Rotation center point</comment>
        <translation type="vanished">その他の座標、</translation>
    </message>
    <message>
        <source>mm</source>
        <translation type="vanished">mm</translation>
    </message>
    <message>
        <source>X:</source>
        <comment>x coordinate</comment>
        <translation type="vanished">X:</translation>
    </message>
    <message>
        <source>Y:</source>
        <comment>y coordinate</comment>
        <translation type="vanished">Y:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="58"/>
        <source>Map coordinate system origin</source>
        <extracomment>Rotation center point</extracomment>
        <translation>地図座標系の原点</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="63"/>
        <source>Georeferencing reference point</source>
        <extracomment>Rotation center point</extracomment>
        <translation>ジオリファレンスの参照点</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="69"/>
        <source>Other point,</source>
        <extracomment>Rotation center point</extracomment>
        <translation>その他の座標、</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="74"/>
        <source>X:</source>
        <translation>X:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="78"/>
        <source>Y:</source>
        <translation>Y:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="82"/>
        <source>Options</source>
        <translation>オプション</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="84"/>
        <source>Adjust georeferencing reference point</source>
        <translation>ジオリファレンスの参照点を調節</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="91"/>
        <source>Adjust georeferencing declination</source>
        <translation>ジオリファレンスの磁気偏角を調節</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_rotate.cpp" line="98"/>
        <source>Rotate non-georeferenced templates</source>
        <translation>ジオリファレンスされていないテンプレートを回転</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::RotatePatternTool</name>
    <message>
        <location filename="../src/tools/rotate_pattern_tool.cpp" line="81"/>
        <source>&lt;b&gt;Angle:&lt;/b&gt; %1° </source>
        <translation>&lt;b&gt;角度:&lt;/b&gt; %1° </translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_pattern_tool.cpp" line="91"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Fixed angles. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 一定角度ずつ回転。 </translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_pattern_tool.cpp" line="85"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Set the direction of area fill patterns or point objects. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: エリアフィルパターンまたはポイントオブジェクトの方向をセットします。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::RotateTool</name>
    <message>
        <source>&lt;b&gt;Rotation:&lt;/b&gt; %1</source>
        <translation type="obsolete">&lt;b&gt;回転:&lt;/b&gt; %1</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the rotation center</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で回転の軸をセットします</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the rotation center, &lt;b&gt;drag&lt;/b&gt; to rotate the selected object(s)</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で回転の軸をセット、&lt;b&gt;ドラッグ&lt;/b&gt;で選択中のオブジェクトを回転します</translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_tool.cpp" line="195"/>
        <source>&lt;b&gt;Rotation:&lt;/b&gt; %1° </source>
        <translation>&lt;b&gt;回転:&lt;/b&gt; %1° </translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_tool.cpp" line="207"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Fixed angles. </source>
        <translation>&lt;b&gt;%1&lt;/b&gt;: 一定角度ずつ回転。 </translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_tool.cpp" line="201"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the center of rotation. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: 回転の軸をセットします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/rotate_tool.cpp" line="202"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Rotate the selected objects. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 選択中のオブジェクトを回転します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ScaleMapDialog</name>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="43"/>
        <source>Change map scale</source>
        <translation>地図の縮尺の変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="47"/>
        <source>Scaling parameters</source>
        <translation>縮尺変更のパラメータ</translation>
    </message>
    <message>
        <source>New scale:  1 :</source>
        <translation type="vanished">新しい縮尺: 1:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="55"/>
        <source>Scaling center:</source>
        <translation>縮尺変更の中心:</translation>
    </message>
    <message>
        <source>Map coordinate system origin</source>
        <comment>Scaling center point</comment>
        <translation type="vanished">地図座標系の原点</translation>
    </message>
    <message>
        <source>Georeferencing reference point</source>
        <comment>Scaling center point</comment>
        <translation type="vanished">ジオリファレンスの参照点</translation>
    </message>
    <message>
        <source>Other point,</source>
        <comment>Scaling center point</comment>
        <translation type="vanished">その他の座標、</translation>
    </message>
    <message>
        <source>mm</source>
        <translation type="vanished">mm</translation>
    </message>
    <message>
        <source>X:</source>
        <comment>x coordinate</comment>
        <translation type="vanished">X:</translation>
    </message>
    <message>
        <source>Y:</source>
        <comment>y coordinate</comment>
        <translation type="vanished">Y:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="53"/>
        <source>New scale:</source>
        <translation>新しいスケール:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="58"/>
        <source>Map coordinate system origin</source>
        <extracomment>Scaling center point</extracomment>
        <translation>地図座標系の原点</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="63"/>
        <source>Georeferencing reference point</source>
        <extracomment>Scaling center point</extracomment>
        <translation>ジオリファレンスの参照点</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="69"/>
        <source>Other point,</source>
        <extracomment>Scaling center point</extracomment>
        <translation>その他の座標、</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="74"/>
        <source>X:</source>
        <translation>X:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="78"/>
        <source>Y:</source>
        <translation>Y:</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="81"/>
        <source>Options</source>
        <translation>オプション</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="83"/>
        <source>Scale symbol sizes</source>
        <translation>記号サイズの縮尺の変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="90"/>
        <source>Scale map object positions</source>
        <translation>オブジェクトの位置を縮尺に合わせて変更</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="97"/>
        <source>Adjust georeferencing reference point</source>
        <translation>ジオリファレンスの参照点を調節</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_dialog_scale.cpp" line="104"/>
        <source>Scale non-georeferenced templates</source>
        <translation>ジオリファレンスされていないテンプレートを縮尺変更</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="obsolete">キャンセル</translation>
    </message>
    <message>
        <source>Adjust</source>
        <translation type="obsolete">変更</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::ScaleTool</name>
    <message>
        <location filename="../src/tools/scale_tool.cpp" line="69"/>
        <source>&lt;b&gt;Scaling:&lt;/b&gt; %1%</source>
        <translation>&lt;b&gt;拡大・縮小率:&lt;/b&gt; %1%</translation>
    </message>
    <message>
        <location filename="../src/tools/scale_tool.cpp" line="75"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the scaling center. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: スケール変更の中心点をセットします。 </translation>
    </message>
    <message>
        <location filename="../src/tools/scale_tool.cpp" line="77"/>
        <source>&lt;b&gt;%1&lt;/b&gt;: Switch to individual object scaling. </source>
        <translation>&lt;b&gt;&gt;%1&lt;/b&gt;: 個々のオブジェクトスケーリングに切り替えます。 </translation>
    </message>
    <message>
        <location filename="../src/tools/scale_tool.cpp" line="73"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Scale the selected objects. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: 選択中のオブジェクトのスケールを変更します。 </translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the scaling center</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で拡大・縮小の中心をセットします</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the scaling center, &lt;b&gt;drag&lt;/b&gt; to scale the selected object(s)</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;で拡大・縮小の中心をセット、&lt;b&gt;ドラッグ&lt;/b&gt;で選択中のオブジェクトを拡大・縮小します</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SelectCRSDialog</name>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="63"/>
        <source>Select coordinate reference system</source>
        <translation>座標参照系を選択</translation>
    </message>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="71"/>
        <source>Same as map</source>
        <translation>地図と同じ</translation>
    </message>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="77"/>
        <source>Local</source>
        <translation>ローカル</translation>
    </message>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="82"/>
        <source>Geographic coordinates (WGS84)</source>
        <translation>地理座標 (WGS84)</translation>
    </message>
    <message>
        <source>(local)</source>
        <translation type="vanished">(local)</translation>
    </message>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="94"/>
        <source>Status:</source>
        <translation>ステータス:</translation>
    </message>
    <message>
        <location filename="../src/gui/select_crs_dialog.cpp" line="139"/>
        <source>valid</source>
        <translation>有効</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SensorsSettingsPage</name>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="52"/>
        <source>Location:</source>
        <translation>場所:</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="55"/>
        <source>Source:</source>
        <translation>ソース:</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="61"/>
        <source>Serial port (NMEA):</source>
        <translation>シリアルポート (NMEA):</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="80"/>
        <source>Sensors</source>
        <translation>センサー</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="112"/>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="149"/>
        <source>Default</source>
        <translation>デフォルト</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="115"/>
        <source>Serial port (NMEA)</source>
        <translation>シリアルポート (NMEA)</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="118"/>
        <source>Windows</source>
        <translation>Windows</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="121"/>
        <source>GeoClue</source>
        <translation>GeoClue</translation>
    </message>
    <message>
        <location filename="../src/sensors/sensors_settings_page.cpp" line="124"/>
        <source>Core Location</source>
        <translation>コアロケーション</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SettingsDialog</name>
    <message>
        <location filename="../src/gui/settings_dialog.cpp" line="78"/>
        <source>Settings</source>
        <translation>設定</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::StorageLocation</name>
    <message>
        <source>&apos;%1&apos; is stored in a regular location.</source>
        <translation type="vanished">&apos;%1&apos; は、通常の場所に格納されます。</translation>
    </message>
    <message>
        <location filename="../src/core/storage_location.cpp" line="308"/>
        <source>&apos;%1&apos; is located in app storage. The files will be removed when uninstalling the app.</source>
        <translation>&apos;%1&apos; は、アプリ ストレージにあります。アプリをアンインストールするときにファイルは削除されます。</translation>
    </message>
    <message>
        <location filename="../src/core/storage_location.cpp" line="311"/>
        <source>&apos;%1&apos; is not writable. Changes cannot be saved.</source>
        <translation>&apos;%1&apos; は書き込み可能ではありません。変更を保存できません。</translation>
    </message>
    <message>
        <location filename="../src/core/storage_location.cpp" line="314"/>
        <source>Extra permissions are required to access &apos;%1&apos;.</source>
        <translation>&apos;%1&apos; にアクセスするには、追加のアクセス許可が必要です。</translation>
    </message>
    <message>
        <location filename="../src/core/storage_location.cpp" line="317"/>
        <source>&apos;%1&apos; is not a valid storage location.</source>
        <translation>&apos;%1&apos; は有効な格納場所ではありません。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolDropDown</name>
    <message>
        <location filename="../src/gui/widgets/symbol_dropdown.cpp" line="56"/>
        <source>- none -</source>
        <translation>- none -</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolDropDownDelegate</name>
    <message>
        <location filename="../src/gui/widgets/symbol_dropdown.cpp" line="170"/>
        <source>- None -</source>
        <translation>- None -</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolPropertiesWidget</name>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="64"/>
        <source>Number:</source>
        <translation>番号:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="75"/>
        <source>Edit</source>
        <translation>編集</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="76"/>
        <source>Name:</source>
        <translation>名前:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="78"/>
        <source>Description:</source>
        <translation>説明:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="80"/>
        <source>Helper symbol (not shown in finished map)</source>
        <translation>補助記号 (完成地図には非表示)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="130"/>
        <source>General</source>
        <translation>一般</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="133"/>
        <source>Icon</source>
        <translation>アイコン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_properties_widget.cpp" line="224"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1381"/>
        <source>Description</source>
        <translation>説明</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolRenderWidget</name>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="104"/>
        <source>For symbols with description, press F1 while the tooltip is visible to show it</source>
        <translation>説明つきの記号では、ツールチップが表示されている間にF1キーを押すと説明を表示</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="108"/>
        <source>New symbol</source>
        <translation>新しい記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="351"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="110"/>
        <source>Point</source>
        <translation>ポイント記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="346"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="111"/>
        <source>Line</source>
        <translation>ライン記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="336"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="112"/>
        <source>Area</source>
        <translation>エリア記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="356"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="113"/>
        <source>Text</source>
        <translation>テキスト記号</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="341"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="114"/>
        <source>Combined</source>
        <translation>組み合わせ記号</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="117"/>
        <source>Edit</source>
        <translation>編集</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="118"/>
        <source>Duplicate</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="119"/>
        <source>Delete</source>
        <translation>削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="120"/>
        <source>Scale...</source>
        <translation>縮尺...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="122"/>
        <source>Copy</source>
        <translation>コピー</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="123"/>
        <source>Paste</source>
        <translation>貼り付け</translation>
    </message>
    <message>
        <source>Switch symbol of selected object(s)</source>
        <translation type="vanished">選択中のオブジェクトの記号を置換</translation>
    </message>
    <message>
        <source>Fill / Create border for selected object(s)</source>
        <translation type="vanished">塗りつぶし/選択中のオブジェクトの輪郭線を作成</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="141"/>
        <source>Select symbols</source>
        <translation>選択中の記号</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="152"/>
        <source>Enable drag and drop</source>
        <translation>ドラッグ＆ドロップを有効にする</translation>
    </message>
    <message>
        <source>All</source>
        <translation type="obsolete">すべて</translation>
    </message>
    <message>
        <source>Unused</source>
        <translation type="obsolete">未使用</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="902"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="916"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="928"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="902"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="928"/>
        <source>An internal error occurred, sorry!</source>
        <translation>内部エラーが発生しました!</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="916"/>
        <source>There are no symbols in clipboard which could be pasted!</source>
        <translation>貼り付けのできる記号がクリップボードにありません!</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1114"/>
        <source>Select all objects with this symbol</source>
        <translation>この記号のオブジェクトをすべて選択する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1115"/>
        <source>Add all objects with this symbol to selection</source>
        <translation>この記号のオブジェクトをすべて選択に追加する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1116"/>
        <source>Remove all objects with this symbol from selection</source>
        <translation>選択範囲から、この記号のすべてのオブジェクトを削除</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1394"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1117"/>
        <source>Hide objects with this symbol</source>
        <translation>この記号のオブジェクトを隠す</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="1405"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1118"/>
        <source>Protect objects with this symbol</source>
        <translation>この記号のオブジェクトを保護</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1123"/>
        <source>Add all objects with selected symbols to selection</source>
        <translation>選択中の記号のオブジェクトをすべて選択に追加する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1124"/>
        <source>Remove all objects with selected symbols from selection</source>
        <translation>選択範囲から、選択された記号のすべてのオブジェクトを削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1125"/>
        <source>Hide objects with selected symbols</source>
        <translation>選択中のオブジェクトを隠す</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1126"/>
        <source>Protect objects with selected symbols</source>
        <translation>選択中のオブジェクトを保護する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="142"/>
        <source>Select all</source>
        <translation>すべてを選択</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="99"/>
        <source>F1</source>
        <comment>Shortcut for displaying the symbol&apos;s description</comment>
        <translation>F1</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="138"/>
        <source>Show custom icons</source>
        <translation>カスタムアイコンを表示する</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="143"/>
        <source>Select unused</source>
        <translation>未使用の記号を選択</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="145"/>
        <source>Invert selection</source>
        <translation>選択を反転</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="148"/>
        <source>Sort symbols</source>
        <translation>記号を並び替え</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="149"/>
        <source>Sort by number</source>
        <translation>番号で並べ替え</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="150"/>
        <source>Sort by primary color</source>
        <translation>色で並べ替え</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="151"/>
        <source>Sort by primary color priority</source>
        <translation>色の優先度で並べ替え</translation>
    </message>
    <message>
        <source>Scale symbol %1</source>
        <translation type="obsolete">記号のスケール %1</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="811"/>
        <source>Scale to percentage:</source>
        <translation>パーセントでスケール変更:</translation>
    </message>
    <message>
        <source>Scale symbol(s)</source>
        <translation type="vanished">縮尺記号</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="840"/>
        <source>Confirmation</source>
        <translation>確認</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="841"/>
        <source>The map contains objects with the symbol &quot;%1&quot;. Deleting it will delete those objects and clear the undo history! Do you really want to do that?</source>
        <translation>地図にこの記号 &quot;%1&quot; を使用したオブジェクトが含まれています。削除するとそれらのオブジェクトも削除され、また、アンドゥ履歴も消去されて元に戻すことができなくなります。本当に削除しますか?</translation>
    </message>
    <message>
        <source>Select all objects with symbol &quot;%1&quot;</source>
        <translation type="obsolete">&quot;%1&quot;の記号のオブジェクトを選択</translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_editor.cpp" line="949"/>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="1122"/>
        <source>Select all objects with selected symbols</source>
        <translation>選択中の記号のオブジェクトを選択</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="125"/>
        <source>Switch symbol of selected objects</source>
        <translation>選択したオブジェクトの記号を切り替える</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="126"/>
        <source>Fill / Create border for selected objects</source>
        <translation>選択したオブジェクトの塗りつぶし / 枠線を作成</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/symbol_render_widget.cpp" line="811"/>
        <source>Scale symbols</source>
        <translation>縮尺記号</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolReplacementDialog</name>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="104"/>
        <source>Replace symbol set</source>
        <translation>記号セットを置き換え</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="106"/>
        <source>Configure how the symbols should be replaced, and which.</source>
        <translation>記号の置き換え方法を構成します。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="109"/>
        <source>Import all new symbols, even if not used as replacement</source>
        <translation>置き換えとして使用しない場合でも、すべての新しい記号をインポート</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="115"/>
        <source>Delete original symbols which are unused after the replacement</source>
        <translation>置き換え後、使用されない元の記号を削除</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="118"/>
        <source>Delete unused colors after the replacement</source>
        <translation>置き換え後、未使用の色を削除</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="164"/>
        <source>Symbol mapping:</source>
        <translation>記号マッピング:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="112"/>
        <source>Keep the symbols&apos; hidden / protected states of the old symbol set</source>
        <translation>記号の非表示 / 古い記号セットの保護された状態を保持</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="135"/>
        <source>Match replacement symbols by symbol number</source>
        <translation>記号番号で一致する記号を置き換え</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="133"/>
        <source>Original</source>
        <translation>元</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="130"/>
        <source>Edit the symbol set ID:</source>
        <translation>記号セット ID を編集:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="133"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="143"/>
        <source>Replacement</source>
        <translation>置き換え</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="137"/>
        <source>Match by symbol name</source>
        <translation>記号名で一致</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="142"/>
        <source>Assign new symbols</source>
        <translation>新しい記号を割り当て</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="143"/>
        <source>Pattern</source>
        <translation>パターン</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="155"/>
        <source>Clear replacements</source>
        <translation>置き換えをクリア</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="127"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="158"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="225"/>
        <source>Open CRT file...</source>
        <translation>CRT ファイルを開く...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="160"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="240"/>
        <source>Save CRT file...</source>
        <translation>CRT ファイルを保存...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="165"/>
        <source>Symbol mapping</source>
        <translation>記号マッピング</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="125"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="224"/>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="239"/>
        <source>CRT file</source>
        <translation>CRT ファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="198"/>
        <source>There are multiple replacements for symbol %1.</source>
        <translation>記号 %1 に複数の置換があります。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="201"/>
        <source>Cannot open file:
%1

%2</source>
        <translation>ファイルを開くことができません:
%1

%2</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="254"/>
        <source>Cannot save file:
%1

%2</source>
        <translation>ファイルを保存できません:
%1

%2</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="270"/>
        <source>The cross reference table has been modified.
Do you want to save your changes?</source>
        <translation>相互参照テーブルは変更されています。
変更内容を保存しますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement_dialog.cpp" line="430"/>
        <source>- None -</source>
        <translation>- None -</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="224"/>
        <source>Choose map file to load symbols from</source>
        <translation>記号をロードする地図を選択してください</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="179"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="189"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="236"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="247"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="256"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="180"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="190"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="237"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="248"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="257"/>
        <source>Cannot load symbol set, aborting.</source>
        <translation>記号セットを読み込みできません。中止します。</translation>
    </message>
    <message>
        <source>Cannot load map file, aborting.</source>
        <translation type="vanished">地図ファイルをロードできません、中断します。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="264"/>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="272"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="265"/>
        <source>The symbol set import generated warnings.</source>
        <translation>記号セットのインポートで警告が生成されました。</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_replacement.cpp" line="273"/>
        <source>The chosen symbol set has a scale of 1:%1, while the map scale is 1:%2. Do you really want to choose this set?</source>
        <translation>選択した記号セットの縮尺は 1:%1 ですが、地図の縮尺は 1:%2 です。このセットを選択しますか?</translation>
    </message>
    <message>
        <source>Cannot load CRT file, aborting.</source>
        <translation type="vanished">CRT ファイルを読み込みできません。中止します。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolSettingDialog</name>
    <message>
        <source>General</source>
        <translation type="obsolete">一般</translation>
    </message>
    <message>
        <source>Number:</source>
        <translation type="obsolete">番号:</translation>
    </message>
    <message>
        <source>Name:</source>
        <translation type="obsolete">名前:</translation>
    </message>
    <message>
        <source>Description:</source>
        <translation type="obsolete">説明:</translation>
    </message>
    <message>
        <source>Helper symbol (not shown in finished map)</source>
        <translation type="obsolete">補助記号 (完成地図には非表示)</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="obsolete">キャンセル</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="obsolete">OK</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="62"/>
        <source>Symbol settings</source>
        <translation>記号設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="104"/>
        <source>Template:</source>
        <translation>テンプレート:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="105"/>
        <source>&lt;b&gt;Template:&lt;/b&gt; </source>
        <translation>&lt;b&gt;テンプレート:&lt;/b&gt; </translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="106"/>
        <source>(none)</source>
        <translation>(none)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="107"/>
        <source>Open...</source>
        <translation>開く...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="110"/>
        <source>Center template...</source>
        <translation>テンプレートの中心...</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="115"/>
        <source>bounding box on origin</source>
        <translation>元の閉じたボックス</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="116"/>
        <source>center of gravity on origin</source>
        <translation>原点の重心</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="234"/>
        <source>Select background color</source>
        <translation>背景色の選択</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="420"/>
        <source>The quick brown fox
takes the routechoice
to jump over the lazy dog
1234567890</source>
        <translation>The quick brown fox
takes the routechoice
to jump over the lazy dog
1234567890
△Aaあぁアァ亜字◎</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/symbol_setting_dialog.cpp" line="495"/>
        <source>- unnamed -</source>
        <translation>- 名称未設定 -</translation>
    </message>
    <message>
        <source>Symbol settings - Please enter a symbol name!</source>
        <translation type="obsolete">記号設定 - 記号の名前を入力してください。</translation>
    </message>
    <message>
        <source>Symbol settings for %1</source>
        <translation type="obsolete">%1 の記号設定</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::SymbolToolTip</name>
    <message>
        <location filename="../src/gui/widgets/symbol_tooltip.cpp" line="201"/>
        <source>No description!</source>
        <translation>説明はありません!</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TagSelectWidget</name>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="63"/>
        <source>Relation</source>
        <translation>関係</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="63"/>
        <source>Key</source>
        <translation>キー</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="63"/>
        <source>Comparison</source>
        <translation>比較</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="63"/>
        <source>Value</source>
        <translation>値</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="92"/>
        <source>Add Row</source>
        <translation>行を追加</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="93"/>
        <source>Remove Row</source>
        <translation>行を削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="100"/>
        <source>Move Up</source>
        <translation>上へ移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tag_select_widget.cpp" line="103"/>
        <source>Move Down</source>
        <translation>下へ移動</translation>
    </message>
    <message>
        <source>Select</source>
        <translation type="vanished">選択</translation>
    </message>
    <message>
        <source>Help</source>
        <translation type="vanished">ヘルプ</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/gui/map/map_find_feature.cpp" line="231"/>
        <source>%n object(s) selected</source>
        <translation>
            <numerusform>%n オブジェクト選択済</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/gui/map/map_find_feature.cpp" line="177"/>
        <location filename="../src/gui/map/map_find_feature.cpp" line="223"/>
        <source>Invalid query</source>
        <translation>無効なクエリ</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TagsDialog</name>
    <message>
        <source>Add</source>
        <translation type="vanished">追加</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation type="vanished">削除</translation>
    </message>
    <message>
        <source>Close</source>
        <translation type="vanished">閉じる</translation>
    </message>
    <message>
        <source>Value</source>
        <translation type="vanished">値</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TagsWidget</name>
    <message>
        <location filename="../src/gui/widgets/tags_widget.cpp" line="55"/>
        <source>Key</source>
        <translation>キー</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tags_widget.cpp" line="55"/>
        <source>Value</source>
        <translation>値</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tags_widget.cpp" line="65"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tags_widget.cpp" line="233"/>
        <source>Key exists</source>
        <translation>キーが存在</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/tags_widget.cpp" line="234"/>
        <source>The key &quot;%1&quot; already exists and must not be used twice.</source>
        <translation>キー &quot;%1&quot; は既に存在します。二度は使用しないでください。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Template</name>
    <message>
        <location filename="../src/templates/template.cpp" line="474"/>
        <source>Find the moved template file</source>
        <translation>移動されたテンプレートを探す</translation>
    </message>
    <message>
        <location filename="../src/templates/template.cpp" line="476"/>
        <source>All files (*.*)</source>
        <translation>すべてのファイル (*.*)</translation>
    </message>
    <message>
        <location filename="../src/templates/template.cpp" line="491"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/templates/template.cpp" line="588"/>
        <location filename="../src/templates/template.cpp" line="616"/>
        <source>No such file.</source>
        <translation>そのようなファイルはありません。</translation>
    </message>
    <message>
        <location filename="../src/templates/template.cpp" line="630"/>
        <source>Is the format of the file correct for this template type?</source>
        <translation>ファイルの形式は、このテンプレートの種類として正しいですか?</translation>
    </message>
    <message>
        <location filename="../src/templates/template.cpp" line="637"/>
        <source>Not enough free memory.</source>
        <translation>メモリが足りません。</translation>
    </message>
    <message>
        <source>Cannot change the template to this file! Is the format of the file correct for this template type?</source>
        <translation type="vanished">テンプレートをこのファイルに変更できません。ファイル形式はテンプレートの形式として正しいですか?</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateAdjustActivity</name>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="69"/>
        <source>Template adjustment</source>
        <translation>テンプレートの調節</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="147"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="147"/>
        <source>Failed to calculate adjustment!</source>
        <translation>調節の計算に失敗しました!</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateAdjustAddTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the template position of the pass point</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でパスポイントのテンプレート位置をセットします</translation>
    </message>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to set the map position of the pass point, &lt;b&gt;Esc&lt;/b&gt; to abort</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でパスポイントのマップ位置をセット、&lt;b&gt;Escキー&lt;/b&gt;で中止します</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="593"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the template position of the pass point. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: パスポイントのテンプレート上の位置をセットします。 </translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="618"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Set the map position of the pass point. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: パスポイントのマップ上の位置をセットします。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateAdjustDeleteTool</name>
    <message>
        <source>&lt;b&gt;Click&lt;/b&gt; to delete pass points</source>
        <translation type="obsolete">&lt;b&gt;クリック&lt;/b&gt;でパスポイントを削除します</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="834"/>
        <source>&lt;b&gt;Click&lt;/b&gt;: Delete pass points. </source>
        <translation>&lt;b&gt;クリック&lt;/b&gt;: パスポイントを削除します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateAdjustMoveTool</name>
    <message>
        <source>&lt;b&gt;Drag&lt;/b&gt; to move pass points</source>
        <translation type="obsolete">&lt;b&gt;ドラッグ&lt;/b&gt;でパスポイントを移動します</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="705"/>
        <source>&lt;b&gt;Drag&lt;/b&gt;: Move pass points. </source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;: パスポイントを移動します。 </translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateAdjustWidget</name>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="203"/>
        <source>Pass points:</source>
        <translation>パスポイント:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="205"/>
        <source>New</source>
        <translation>新規</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="209"/>
        <source>Move</source>
        <translation>移動</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="213"/>
        <source>Delete</source>
        <translation>削除</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="220"/>
        <source>Template X</source>
        <translation>テンプレート-X</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="220"/>
        <source>Template Y</source>
        <translation>テンプレート-Y</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="220"/>
        <source>Map X</source>
        <translation>マップ X</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="220"/>
        <source>Map Y</source>
        <translation>マップ Y</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="220"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="231"/>
        <source>Apply pass points</source>
        <translation>パスポイントを適用</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="233"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="234"/>
        <source>Apply &amp;&amp; clear all</source>
        <translation>適用 &amp;&amp; すべて消去</translation>
    </message>
    <message>
        <location filename="../src/templates/template_adjust.cpp" line="235"/>
        <source>Clear all</source>
        <translation>すべて消去</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateGPS</name>
    <message>
        <source>Error</source>
        <translation type="obsolete">エラー</translation>
    </message>
    <message>
        <source>The path is empty, there is nothing to import!</source>
        <translation type="obsolete">path が空白です、インポートするものが何もありません。</translation>
    </message>
    <message>
        <source>Question</source>
        <translation type="obsolete">質問</translation>
    </message>
    <message>
        <source>Should the waypoints be imported as a line going through all points?</source>
        <translation type="obsolete">ウェイポイントを、すべてのポイントを通過しているラインとしてインポートしますか？</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateImage</name>
    <message>
        <location filename="../src/gdal/gdal_image_reader.cpp" line="124"/>
        <location filename="../src/templates/template_image.cpp" line="183"/>
        <source>Not enough free memory (image size: %1x%2 pixels)</source>
        <translation>十分な空きメモリがありません (画像サイズ: %1x%2 ピクセル)</translation>
    </message>
    <message>
        <location filename="../src/gdal/gdal_template.cpp" line="91"/>
        <location filename="../src/templates/template_image.cpp" line="206"/>
        <source>Georeferencing not found</source>
        <translation>ジオリファレンスが見つかりません</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation type="vanished">警告</translation>
    </message>
    <message>
        <source>Loading a GIF image template.
Saving GIF files is not supported. This means that drawings on this template won&apos;t be saved!
If you do not intend to draw on this template however, that is no problem.</source>
        <translation type="vanished">GIFイメージのテンプレートを読み込もうとしています。
GIFファイルの保存はサポートされていません。
テンプレート上へ書き込みを行った場合、保存されず終了時に破棄されることになります。</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image.cpp" line="262"/>
        <location filename="../src/templates/template_image.cpp" line="379"/>
        <source>Select the coordinate reference system of the coordinates in the world file</source>
        <translation>ワールドファイル内の座標の座標参照系を選択してください</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateImageOpenDialog</name>
    <message>
        <source>Open image template</source>
        <translation type="obsolete">画像テンプレートを開く</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="50"/>
        <source>Opening %1</source>
        <translation>%1 を開きます</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="52"/>
        <source>Image size:</source>
        <translation>画像のサイズ:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="55"/>
        <source>Specify how to position or scale the image:</source>
        <translation>画像の位置やサイズの調節方法を指定してください:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="70"/>
        <source>World file</source>
        <translation>ワールドファイル</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="71"/>
        <source>GeoTIFF</source>
        <translation>GeoTIFF</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="72"/>
        <source>no georeferencing information</source>
        <translation>ジオリファレンス情報がありません</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="77"/>
        <source>Georeferenced (%1)</source>
        <translation>ジオリファレンス済 (%1)</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="80"/>
        <source>Meters per pixel:</source>
        <translation>メートル/pixel:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="84"/>
        <source>Scanned with</source>
        <translation>解像度</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="87"/>
        <source>dpi</source>
        <translation>dpi</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="89"/>
        <source>Template scale:  1 :</source>
        <translation>テンプレートの縮尺: 1:</translation>
    </message>
    <message>
        <source>Different template scale 1 :</source>
        <translation type="obsolete">異なったテンプレートのスケール 1 :</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="115"/>
        <source>Cancel</source>
        <translation>キャンセル</translation>
    </message>
    <message>
        <location filename="../src/templates/template_image_open_dialog.cpp" line="116"/>
        <source>Open</source>
        <translation>開く</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateListWidget</name>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="177"/>
        <source>Show</source>
        <translation>表示</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="176"/>
        <source>Opacity</source>
        <translation>不透明度</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="176"/>
        <source>Group</source>
        <translation>グループ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="176"/>
        <source>Filename</source>
        <translation>ファイル名</translation>
    </message>
    <message>
        <source>Create...</source>
        <translation type="obsolete">作成...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="226"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="515"/>
        <source>Sketch</source>
        <translation>スケッチ</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="228"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="519"/>
        <source>GPS</source>
        <translation>GPS</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="221"/>
        <source>Open...</source>
        <translation>開く...</translation>
    </message>
    <message>
        <source>Georeferenced: %1</source>
        <translation type="vanished">ジオリファレンス: %1</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation type="vanished">削除</translation>
    </message>
    <message>
        <source>Close</source>
        <translation type="vanished">閉じる</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="224"/>
        <source>Duplicate</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="232"/>
        <location filename="../src/templates/template_tool_paint.cpp" line="465"/>
        <source>Add template...</source>
        <translation>テンプレートを追加...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="236"/>
        <source>Remove</source>
        <translation>削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="242"/>
        <source>Move Up</source>
        <translation>上へ移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="244"/>
        <source>Move Down</source>
        <translation>下へ移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="266"/>
        <source>Import and remove</source>
        <translation>インポートと削除</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="305"/>
        <source>Help</source>
        <translation>ヘルプ</translation>
    </message>
    <message>
        <source>Selected template(s)</source>
        <translation type="obsolete">選択中のテンプレート</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="251"/>
        <source>Move by hand</source>
        <translation>ドラッグで移動</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="411"/>
        <source>Open image, GPS track or DXF file</source>
        <translation>イメージ、GPSトラック、DXFファイルを開く</translation>
    </message>
    <message>
        <source>Georeference...</source>
        <translation type="obsolete">ジオリファレンス...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="263"/>
        <source>Positioning...</source>
        <translation>位置調整...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="414"/>
        <source>Template files</source>
        <translation>テンプレートファイル</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="414"/>
        <source>All files</source>
        <translation>すべてのファイル</translation>
    </message>
    <message>
        <source>More...</source>
        <translation type="obsolete">もっと表示...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="256"/>
        <source>Adjust...</source>
        <translation>調節...</translation>
    </message>
    <message>
        <source>Numeric transformation window</source>
        <translation type="obsolete">数値変形ウィンドウ</translation>
    </message>
    <message>
        <source>Set transparent color...</source>
        <translation type="obsolete">透明色の設定...</translation>
    </message>
    <message>
        <source>Trace lines...</source>
        <translation type="obsolete">ラインをトレース...</translation>
    </message>
    <message>
        <source>Open image or GPS track ...</source>
        <translation type="obsolete">画像を開く、またはGPSトラック...</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="465"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="787"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1010"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1015"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1056"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1107"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <source>Cannot open template:
%1

File format not recognized.</source>
        <translation type="vanished">テンプレートを開くことができません:
%1

ファイル形式を認識することができません。</translation>
    </message>
    <message>
        <source>Cannot open template:
%1

Failed to load template. Does the file exist and is it valid?</source>
        <translation type="vanished">テンプレートを開くことができません:
%1

テンプレートを開くことに失敗しました。そのファイルは存在して、有効な形式ですか?</translation>
    </message>
    <message>
        <source>Please enter a valid number from 0 to 1, or specify a percentage from 0 to 100!</source>
        <translation type="obsolete">0 から 1 の有効な数字を入力してください。または 0 から 100 のパーセントで指定してください！</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="787"/>
        <source>Please enter a valid integer number to set a group or leave the field empty to ungroup the template!</source>
        <translation>有効な整数値を入力してテンプレートのグループをセットするか、空白のままにしてテンプレートのグループを解除してください！</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1036"/>
        <source>Don&apos;t scale</source>
        <translation>縮尺を変えない</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1031"/>
        <source>Scale by nominal map scale ratio (%1 %)</source>
        <translation>公称地図縮尺比 (%1 %) によって縮尺</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="261"/>
        <source>Georeferenced</source>
        <translation>ジオリファレンス</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="464"/>
        <location filename="../src/templates/template.cpp" line="487"/>
        <source>Cannot open template
%1:
%2</source>
        <translation>テンプレートを開くことができません
%1:
%2</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="431"/>
        <source>File format not recognized.</source>
        <translation>ファイル形式を認識できません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="269"/>
        <source>Vectorize lines</source>
        <translation type="unfinished">ラインをベクトル化</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="444"/>
        <source>Failed to load template. Does the file exist and is it valid?</source>
        <translation>テンプレートを読み込めませんでした。ファイルが存在して、有効ですか?</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1010"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1056"/>
        <source>Cannot load map file, aborting.</source>
        <translation>地図ファイルを読み込みできません、中断します。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1020"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1020"/>
        <source>The map import generated warnings.</source>
        <translation>地図のインポート時に警告が発生しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1033"/>
        <source>Scale by current template scaling (%1 %)</source>
        <translation>現在テンプレートの縮尺 (%1 %) で縮尺</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1038"/>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1068"/>
        <source>Template import</source>
        <translation>テンプレートをインポート</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1039"/>
        <source>How shall the symbols of the imported template map be scaled?</source>
        <translation>インポートされたテンプレート地図の記号の縮尺方法は?</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1069"/>
        <source>The template will be invisible in the overprinting simulation. Switch to normal view?</source>
        <translation>テンプレートは、重ね印刷のシミュレーションでは表示されません。標準表示に切り替えますか?</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1107"/>
        <source>Cannot change the georeferencing state.</source>
        <translation>ジオリファレンス状態を変更できません。</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/template_list_widget.cpp" line="1235"/>
        <source>- Map -</source>
        <translation>- 地図 -</translation>
    </message>
    <message>
        <source>Find the moved template file</source>
        <translation type="obsolete">移動されたテンプレートを探す</translation>
    </message>
    <message>
        <source>All files (*.*)</source>
        <translation type="obsolete">すべてのファイル (*.*)</translation>
    </message>
    <message>
        <source>Cannot change the template to this file! Is the format of the file correct for this template type?</source>
        <translation type="obsolete">テンプレートをこのファイルに変更できません。ファイル形式はテンプレートの形式として正しいですか?</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateMap</name>
    <message>
        <location filename="../src/templates/template_map.cpp" line="118"/>
        <source>Cannot load map file, aborting.</source>
        <translation>地図ファイルを読み込みできません、中断します。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateMoveTool</name>
    <message>
        <location filename="../src/templates/template_tool_move.cpp" line="48"/>
        <source>&lt;b&gt;Drag&lt;/b&gt; to move the current template</source>
        <translation>&lt;b&gt;ドラッグ&lt;/b&gt;で選択中のテンプレートを水平移動します</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplatePositionDockWidget</name>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="39"/>
        <source>Positioning</source>
        <translation>位置調整</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="43"/>
        <source>X:</source>
        <translation>X:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="46"/>
        <source>Y:</source>
        <translation>Y:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="49"/>
        <source>X-Scale:</source>
        <translation>X縮尺:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="52"/>
        <source>Y-Scale:</source>
        <translation>Y縮尺:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="55"/>
        <source>Rotation:</source>
        <translation>回転:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_position_dock_widget.cpp" line="58"/>
        <source>Shear:</source>
        <translation>せん断:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplatePositioningDialog</name>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="42"/>
        <source>Track scaling and positioning</source>
        <translation>トラックの縮尺と測位</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="47"/>
        <source>Coordinate system</source>
        <translation>座標系</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="48"/>
        <source>Real</source>
        <translation>実際</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="49"/>
        <source>Map</source>
        <translation>地図</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="52"/>
        <source>m</source>
        <comment>meters</comment>
        <translation>m</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="55"/>
        <source>One coordinate unit equals:</source>
        <translation>1 座標単位に等しい:</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="57"/>
        <source>Position track at given coordinates</source>
        <translation>指定した座標でトラックを配置</translation>
    </message>
    <message>
        <location filename="../src/templates/template_positioning_dialog.cpp" line="61"/>
        <source>Position track at view center</source>
        <translation>表示の中心でトラックを配置</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TemplateTrack</name>
    <message>
        <location filename="../src/templates/template_track.cpp" line="169"/>
        <location filename="../src/templates/template_track.cpp" line="175"/>
        <source>This template must be loaded with GDAL/OGR.</source>
        <translation>このテンプレートは GDAL/OGR で読み込む必要があります。</translation>
    </message>
    <message>
        <source>Select the coordinate reference system of the track coordinates</source>
        <translation type="vanished">トラック座標の座標参照系を選択</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="207"/>
        <source>Opening track ...</source>
        <translation>トラックを開いています ...</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="208"/>
        <source>Load the track in georeferenced or non-georeferenced mode?</source>
        <translation>ジオリファレンスまたは非ジオリファレンスモードでトラックを読み込みますか?</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="210"/>
        <source>Positions the track according to the map&apos;s georeferencing settings.</source>
        <translation>地図のジオリファレンス設定に従ってトラックを配置します。</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="212"/>
        <source>These are not configured yet, so they will be shown as the next step.</source>
        <translation>これらはまだ構成されていません。次のステップとして表示されます。</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="213"/>
        <source>Georeferenced</source>
        <translation>ジオリファレンス</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="214"/>
        <source>Non-georeferenced</source>
        <translation>非ジオリファレンス</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="214"/>
        <source>Projects the track using an orthographic projection with center at the track&apos;s coordinate average. Allows adjustment of the transformation and setting the map georeferencing using the adjusted track position.</source>
        <translation>トラックの座標の平均で中心とした正射影を使用してトラックを投影します。調整後のトラックの位置を使用して、変換の調整と地図ジオリファレンスの設定を可能にします。</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="417"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="417"/>
        <source>The path is empty, there is nothing to import!</source>
        <translation>パスが空です、インポートするものが何もありません!</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="430"/>
        <source>Question</source>
        <translation>質問</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="430"/>
        <source>Should the waypoints be imported as a line going through all points?</source>
        <translation>ウェイポイントを、すべてのポイントを通過しているラインとしてインポートしますか？</translation>
    </message>
    <message>
        <location filename="../src/templates/template_track.cpp" line="485"/>
        <source>Import problems</source>
        <translation>インポートの問題</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/templates/template_track.cpp" line="486"/>
        <source>%n path object(s) could not be imported (reason: missing coordinates).</source>
        <translation>
            <numerusform>%n コース オブジェクトをインポートできませんでした (理由: 座標が見つかりません)。</numerusform>
        </translation>
    </message>
    <message>
        <source>Error reading</source>
        <translation type="vanished">読み込みエラー</translation>
    </message>
    <message>
        <source>There was an error reading the DXF file %1:

%1</source>
        <translation type="obsolete">DXF ファイル %1 を読み込む際にエラーが発生しました。

%1</translation>
    </message>
    <message>
        <source>There was an error reading the DXF file %1:

%2</source>
        <translation type="vanished">DXF ファイル:%1 を読み込む際にエラーが発生しました。

%2</translation>
    </message>
    <message>
        <source>%1:
Not an OSM file.</source>
        <translation type="vanished">%1:
OSM ファイルではありません。</translation>
    </message>
    <message>
        <source>The OSM file has version %1.
The minimum supported version is %2.</source>
        <translation type="vanished">OSM ファイルのバージョンは %1 です。
サポートされているバージョンは%2 以上です。</translation>
    </message>
    <message>
        <source>The OSM file has version %1.
The maximum supported version is %2.</source>
        <translation type="vanished">OSM ファイルのバージョンは %1 です。
サポートされているバージョンは%2 以下です。</translation>
    </message>
    <message>
        <source>Problems</source>
        <translation type="vanished">問題</translation>
    </message>
    <message>
        <source>%1 nodes could not be processed correctly.</source>
        <translation type="vanished">%1 ノードを正しく処理できませんでした。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TextBrowserDialog</name>
    <message>
        <source>External link: %1</source>
        <translation type="vanished">外部リンク: %1</translation>
    </message>
    <message>
        <source>Click to view</source>
        <translation type="vanished">クリックすると表示</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TextObjectAlignmentDockWidget</name>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="62"/>
        <source>Alignment</source>
        <translation>整列</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="45"/>
        <source>Left</source>
        <translation>左</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="46"/>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="52"/>
        <source>Center</source>
        <translation>中央</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="47"/>
        <source>Right</source>
        <translation>右</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="51"/>
        <source>Top</source>
        <translation>上</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="53"/>
        <source>Baseline</source>
        <translation>ベースライン</translation>
    </message>
    <message>
        <location filename="../src/gui/widgets/text_alignment_widget.cpp" line="54"/>
        <source>Bottom</source>
        <translation>底</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::TextSymbolSettings</name>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="95"/>
        <source>Text settings</source>
        <translation>テキスト設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="107"/>
        <source>Font family:</source>
        <translation>フォント名:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="111"/>
        <source>Font size:</source>
        <translation>フォントサイズ:</translation>
    </message>
    <message>
        <source>Determine size...</source>
        <translation type="vanished">サイズの決定...</translation>
    </message>
    <message>
        <location filename="../src/core/symbols/text_symbol.cpp" line="525"/>
        <source>A</source>
        <comment>First capital letter of the local alphabet</comment>
        <translation>A</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="125"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="150"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="210"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="216"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="490"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="508"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="602"/>
        <source>mm</source>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="110"/>
        <source>pt</source>
        <translation>pt</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="133"/>
        <source>Text color:</source>
        <translation>テキストの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="136"/>
        <source>bold</source>
        <translation>太字</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="138"/>
        <source>italic</source>
        <translation>斜体</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="140"/>
        <source>underlined</source>
        <translation>下線</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="143"/>
        <source>Text style:</source>
        <translation>文字の修飾:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="147"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="153"/>
        <source>%</source>
        <translation>%</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="148"/>
        <source>Line spacing:</source>
        <translation>行間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="151"/>
        <source>Paragraph spacing:</source>
        <translation>段落間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="154"/>
        <source>Character spacing:</source>
        <translation>文字間隔:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="156"/>
        <source>Kerning</source>
        <translation>文字間隔の自動調整 (カーニング)</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="163"/>
        <source>Symbol icon text:</source>
        <translation>記号アイコンのテキスト:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="167"/>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="175"/>
        <source>Framing</source>
        <translation>フレーミング</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="170"/>
        <source>OCAD compatibility settings</source>
        <translation>OCAD 互換の設定</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="181"/>
        <source>Framing color:</source>
        <translation>フレーミングの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="183"/>
        <source>Line framing</source>
        <translation>ラインフレーミング</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="189"/>
        <source>Shadow framing</source>
        <translation>シャドーフレーミング</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="193"/>
        <source>Left/Right Offset:</source>
        <translation>左/右のオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="196"/>
        <source>Top/Down Offset:</source>
        <translation>上/下のオフセット:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="200"/>
        <source>OCAD compatibility</source>
        <translation>OCAD 互換</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="207"/>
        <source>enabled</source>
        <translation>有効</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="211"/>
        <source>Line width:</source>
        <translation>ラインの幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="214"/>
        <source>Line color:</source>
        <translation>ラインの色:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="490"/>
        <source>Position:</source>
        <translation>位置:</translation>
    </message>
    <message>
        <source>Paragraph spacing [mm]:</source>
        <translation type="obsolete">段落間隔[mm]:</translation>
    </message>
    <message>
        <source>Character spacing [percentage of space character]:</source>
        <translation type="obsolete">文字間隔 [空白文字の割合]:</translation>
    </message>
    <message>
        <source>use kerning</source>
        <translation type="obsolete">文字間隔の調整</translation>
    </message>
    <message>
        <source>Show OCAD compatibility settings</source>
        <translation type="obsolete">OCAD互換の設定の表示</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="205"/>
        <source>Line below paragraphs</source>
        <translation>下線</translation>
    </message>
    <message>
        <source>Enable</source>
        <translation type="obsolete">有効にする</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="187"/>
        <source>Width:</source>
        <translation>幅:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="217"/>
        <source>Distance from baseline:</source>
        <translation>文字のベースラインからの距離:</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="221"/>
        <source>Custom tabulator positions</source>
        <translation>カスタムタビュレータの位置</translation>
    </message>
    <message>
        <location filename="../src/gui/symbols/text_symbol_settings.cpp" line="489"/>
        <source>Add custom tabulator</source>
        <translation>カスタムタビュレータの追加</translation>
    </message>
    <message>
        <source>Position [mm]:</source>
        <translation type="obsolete">位置 [mm]:</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::UTMZoneEdit</name>
    <message>
        <location filename="../src/gui/widgets/crs_param_widgets.cpp" line="77"/>
        <source>Calculate</source>
        <translation>計算</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::UndoManager</name>
    <message>
        <location filename="../src/undo/undo_manager.cpp" line="140"/>
        <location filename="../src/undo/undo_manager.cpp" line="185"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/undo/undo_manager.cpp" line="140"/>
        <source>Cannot undo because the last undo step became invalid. This can for example happen if you change the symbol of an object to another and then delete the old symbol.</source>
        <translation>直前の操作が無効なので、元に戻すことができません。これは例えば、オブジェクトの記号を変更した後で、前の記号を削除したような場合に起こります。</translation>
    </message>
    <message>
        <location filename="../src/undo/undo_manager.cpp" line="147"/>
        <source>Confirmation</source>
        <translation>確認</translation>
    </message>
    <message>
        <location filename="../src/undo/undo_manager.cpp" line="147"/>
        <source>Undoing this step will go beyond the point where the file was loaded. Are you sure?</source>
        <translation>ファイルを開いた時点より前の操作を元に戻します。よろしいですか？</translation>
    </message>
    <message>
        <location filename="../src/undo/undo_manager.cpp" line="185"/>
        <source>Cannot redo because the first redo step became invalid. This can for example happen if you delete the symbol of an object you have drawn.</source>
        <translation>直後の操作が無効なので、やり直すことができません。これは例えば、描画したオブジェクトの記号を削除したような場合に起こります。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::UnitOfMeasurement</name>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="202"/>
        <source>mm</source>
        <comment>millimeters</comment>
        <translation>mm</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="207"/>
        <source>m</source>
        <comment>meters</comment>
        <translation>m</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="212"/>
        <source>°</source>
        <comment>degrees</comment>
        <translation>°</translation>
    </message>
    <message>
        <location filename="../src/tools/tool_helpers.cpp" line="626"/>
        <source>%1°</source>
        <comment>degree</comment>
        <translation>%1°</translation>
    </message>
    <message>
        <location filename="../src/tools/tool_helpers.cpp" line="627"/>
        <source>%1 m</source>
        <comment>meter</comment>
        <translation>%1 m</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::Util</name>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="128"/>
        <location filename="../src/gui/util_gui.cpp" line="142"/>
        <location filename="../src/gui/util_gui.cpp" line="176"/>
        <source>Error</source>
        <translation>エラー</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="128"/>
        <source>Failed to locate the help files.</source>
        <translation>ヘルプファイルを探すのに失敗しました。</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="142"/>
        <source>Failed to locate the help browser (&quot;Qt Assistant&quot;).</source>
        <translation>ヘルプブラウザを探すのに失敗しました (&quot;QT Assistant&quot;)。</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="177"/>
        <source>Failed to launch the help browser (&quot;Qt Assistant&quot;).</source>
        <translation>ヘルプブラウザの開始に失敗しました(&quot;Qt Assistant&quot;)。</translation>
    </message>
    <message>
        <location filename="../src/gui/util_gui.cpp" line="195"/>
        <source>See more...</source>
        <extracomment>This &quot;See more&quot; is displayed as a link to the manual in What&apos;s-this tooltips.</extracomment>
        <translation>さらに表示...</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::XMLFileExporter</name>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="243"/>
        <source>Older versions of Mapper do not support multiple map parts. To save the map in compatibility mode, you must first merge all map parts.</source>
        <translation>Mapper の古いバージョンでは、複数の地図パートはサポートしていません。互換モードで地図を保存するには、最初にすべての地図パートをマージする必要があります。</translation>
    </message>
</context>
<context>
    <name>OpenOrienteering::XMLFileImporter</name>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="528"/>
        <source>Unsupported element: %1 (line %2 column %3)</source>
        <translation>サポートされていない要素: %1 (%2 行目 %3 列目)</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="572"/>
        <source>Some coordinates were out of bounds for printing. Map content was adjusted.</source>
        <translation>いくつかの座標は印刷範囲外でした。地図コンテンツを調整しました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="662"/>
        <source>unknown</source>
        <translation>不明</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="663"/>
        <source>Parts of this file cannot be read by this version of Mapper. Minimum required version: %1</source>
        <translation>このバージョンの Mapper では、このファイルの要素を読み取ることができません。最低限必要なバージョン: %1</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="644"/>
        <source>Error at line %1 column %2: %3</source>
        <translation>%1 行目 %2 列目でエラー: %3</translation>
    </message>
    <message>
        <source>The map notes could not be read.</source>
        <translation type="vanished">ノートを読み取ることができません。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="680"/>
        <location filename="../src/fileformats/xml_file_format.cpp" line="942"/>
        <source>Some invalid characters had to be removed.</source>
        <translation>いくつかの無効な文字を削除する必要がありました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="698"/>
        <source>Unknown error</source>
        <translation>不明なエラー</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="699"/>
        <source>Unsupported or invalid georeferencing specification &apos;%1&apos;: %2</source>
        <translation>サポートされていないか無効なジオリファレンス仕様 &apos;%1&apos;: %2</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="835"/>
        <location filename="../src/fileformats/xml_file_format.cpp" line="888"/>
        <source>Could not set knockout property of color &apos;%1&apos;.</source>
        <translation>色 &apos;%1&apos; のノックアウト プロパティを設定できませんでした。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="848"/>
        <source>Expected %1 colors, found %2.</source>
        <translation>%1 の色が必要ですが、%2 が見つかりました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="864"/>
        <source>Spot color %1 not found while processing %2 (%3).</source>
        <translation>%2 (%3) の処理中に、スポット カラー %1 が見つかりませんでした。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="920"/>
        <source>Expected %1 symbols, found %2.</source>
        <translation>%1 の記号を予期しましたが、%2 が見つかりました。</translation>
    </message>
    <message>
        <location filename="../src/fileformats/xml_file_format.cpp" line="959"/>
        <source>Expected %1 map parts, found %2.</source>
        <translation>%1 の地図の要素が必要ですが、%2 が見つかりました。</translation>
    </message>
</context>
</TS>

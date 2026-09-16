"""Production ordering/storage logic; fake containers model recycling and restart."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class AppOrderTests(unittest.TestCase):
    def test_identity_recycling_restart_and_partial_order(self):
        compiler = shutil.which('g++')
        if not compiler:
            self.skipTest('g++ unavailable')
        source = (Path(__file__).resolve().parents[1] / 'taskbar-split.wh.cpp').read_text()
        functions = source[source.index('std::wstring ButtonAppId'):source.index('struct Placement')]
        harness = r'''
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_set>
#include <map>
#include <cassert>
using FrameworkElement = int;
namespace winrt {
template<class T> struct weak_ref { T v; weak_ref(T x):v(x){} T get() const{return v;} };
}
struct ButtonInfo { int element; double width; bool running; std::wstring appId; };
std::vector<winrt::weak_ref<FrameworkElement>> g_runningOrder, g_pinnedOrder;
std::vector<std::wstring> g_runningAppOrder, g_pinnedAppOrder;
std::unordered_set<std::wstring> g_previousPinnedApps;
bool g_orderLoaded = false;
std::map<int,std::wstring> automationIds;
namespace Automation { struct AutomationProperties {
    static std::wstring GetAutomationId(int e) { return automationIds[e]; }
}; }
std::map<std::wstring,std::wstring> storage;
size_t Wh_GetStringValue(wchar_t const* key, wchar_t* out, size_t size) {
    auto value=storage[key]; if(value.size()+1>size) return 0;
    std::copy(value.begin(),value.end(),out);out[value.size()]=0;return value.size();
}
bool Wh_SetStringValue(wchar_t const* key,wchar_t const* value){storage[key]=value;return true;}
void Wh_Log(wchar_t const*){}
''' + functions + r'''
int main() {
    automationIds[1]=L"Appid: example.app";
    automationIds[2]=L"Appid: C:\\Program Files\\app.exe";
    automationIds[3]=L"Window:12345";
    assert(ButtonAppId(1)==L"example.app");
    assert(ButtonAppId(2)==L"C:\\Program Files\\app.exe");
    assert(ButtonAppId(3).empty());
    automationIds[1]=L"Appid: different.app";
    assert(ButtonAppId(1)==L"different.app");
    ButtonInfo a{1,44,false,L"a"}, b{2,44,false,L"b"}, c{3,44,false,L"c"};
    LoadAppOrders();
    std::vector<ButtonInfo*> items{&a,&b,&c};
    OrderPinnedButtons(items);
    MergeVisibleAppOrder(g_pinnedAppOrder,{L"b",L"c",L"a"});
    SaveAppOrder(false);
    for(int i=0;i<99;++i){
        items={&a,&b,&c}; OrderPinnedButtons(items);
        assert((items==std::vector<ButtonInfo*>{&b,&c,&a}));
    }
    // New XAML objects for the same apps retain order.
    a.element=100; b.element=101; c.element=102;
    items={&a,&b,&c}; OrderPinnedButtons(items);
    assert((items==std::vector<ButtonInfo*>{&b,&c,&a}));
    // Recycling b's old object for another app does not inherit b's rank.
    b.appId=L"different"; items={&a,&b,&c}; OrderPinnedButtons(items);
    assert((items==std::vector<ButtonInfo*>{&c,&a,&b}));
    // Simulated Explorer restart reads persistent IDs, not live references.
    g_pinnedOrder.clear();g_pinnedAppOrder.clear();g_orderLoaded=false;
    b.appId=L"b";LoadAppOrders();items={&a,&b,&c};OrderPinnedButtons(items);
    assert((items==std::vector<ButtonInfo*>{&b,&c,&a}));
    // Missing/overflow apps retain their slots when visible subset reorders.
    std::vector<std::wstring> partial{L"a",L"hidden",L"b",L"c"};
    MergeVisibleAppOrder(partial,{L"c",L"b",L"a"});
    assert((partial==std::vector<std::wstring>{L"c",L"hidden",L"b",L"a"}));
    // Aborted gestures restore their snapshot and do not write storage.
    auto snapshot=g_pinnedAppOrder;auto disk=storage;
    MergeVisibleAppOrder(g_pinnedAppOrder,{L"a",L"b",L"c"});
    g_pinnedAppOrder=snapshot;assert(storage==disk);
    // Pinned -> running app appends, even if it had an older saved rank.
    g_runningAppOrder={L"a",L"c"};g_previousPinnedApps={L"a"};
    items={&a,&c};OrderRunningButtons(items);
    assert((items==std::vector<ButtonInfo*>{&c,&a}));
    // Unknown IDs are never stored or promoted by a recycled pointer.
    b.appId=L"";items={&b,&a};OrderPinnedButtons(items);
    assert(items.back()==&b);
    // App IDs can contain delimiters and Unicode. Malformed storage is rejected.
    std::vector<std::wstring> ids{L"x:y;z\n",L"\u0142\u00f3d\u017a",L"C:\\app.exe"};
    assert(DecodeAppOrder(EncodeAppOrder(ids))==ids);
    for(auto bad:{L"",L"2;1:a",L"1;99999:a",L"1;3:a",L"1;0:",L"1;x:a"})
        assert(DecodeAppOrder(bad).empty());
    assert((DecodeAppOrder(L"1;1:a1:a")==std::vector<std::wstring>{L"a"}));
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / 'app-order')
            result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror',
                                     '-x', 'c++', '-', '-o', executable],
                                    input=harness, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([executable], check=True, capture_output=True)

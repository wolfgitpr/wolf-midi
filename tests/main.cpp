#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <wolf-midi/MidiFile.h>

namespace
{
    int g_checks = 0;
    int g_failures = 0;

    void check(const bool condition, const std::string &what) {
        ++g_checks;
        if (condition) {
            std::cout << "[  OK  ] " << what << std::endl;
        } else {
            ++g_failures;
            std::cout << "[ FAIL ] " << what << std::endl;
        }
    }

    std::vector<char> toBytes(const std::string &text) {
        return {text.begin(), text.end()};
    }

    std::string readFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    bool containsEvent(const Midi::MidiFile &file, const Midi::MidiEvent::EventType type, const int32_t tick,
                       const int voice, const int note, const int value) {
        for (const auto *e : file.events()) {
            if (e->type() != type || e->tick() != tick || e->voice() != voice) {
                continue;
            }
            if (type == Midi::MidiEvent::NoteOn || type == Midi::MidiEvent::NoteOff ||
                type == Midi::MidiEvent::KeyPressure) {
                if (e->note() == note && (value < 0 || e->velocity() == value || e->amount() == value)) {
                    return true;
                }
            } else if (type == Midi::MidiEvent::ControlChange || type == Midi::MidiEvent::ProgramChange) {
                if (e->number() == note && (value < 0 || e->value() == value)) {
                    return true;
                }
            } else if (type == Midi::MidiEvent::PitchWheel || type == Midi::MidiEvent::ChannelPressure) {
                if (e->value() == value || e->amount() == value) {
                    return true;
                }
            }
        }
        return false;
    }

    int countType(const Midi::MidiFile &file, const Midi::MidiEvent::EventType type) {
        int count = 0;
        for (const auto *e : file.events()) {
            if (e->type() == type) {
                ++count;
            }
        }
        return count;
    }

    void buildSampleMidiFile(Midi::MidiFile &midi) {
        midi.setFileFormat(1);
        midi.setResolution(480);
        midi.createTrack(); // track 0: meta events
        midi.createTrack(); // track 1: channel events

        midi.createTempoEvent(0, 0, 120.0f);
        midi.createTimeSignatureEvent(0, 0, 4, 4);
        midi.createMetaEvent(0, 0, Midi::MidiEvent::TrackName, toBytes("wolf-midi"));
        midi.createMarkerEvent(0, 240, toBytes("marker"));

        midi.createNoteOnEvent(1, 0, 0, 60, 100);
        midi.createNoteOffEvent(1, 480, 0, 60);
        midi.createControlChangeEvent(1, 240, 0, 7, 100);
        midi.createProgramChangeEvent(1, 120, 0, 5);
        midi.createPitchWheelEvent(1, 360, 0, 4096);
        midi.createKeyPressureEvent(1, 60, 0, 60, 33);
        midi.createChannelPressureEvent(1, 90, 0, 44);
        midi.createLyricEvent(1, 60, toBytes("la"));
        midi.createSysexEvent(1, 30, {'\xF0', '\x7E', '\x00'});
    }

    void testFileRoundTrip(const std::filesystem::path &midiPath, const std::filesystem::path &copyPath) {
        Midi::MidiFile midi;
        buildSampleMidiFile(midi);
        const auto eventCount = midi.events().size();

        check(midi.save(midiPath), "save to file");
        check(std::filesystem::exists(midiPath), "saved file exists");
        check(readFile(midiPath).size() > 14, "saved file is not empty");

        Midi::MidiFile loaded;
        check(loaded.load(midiPath), "load from file");
        check(loaded.fileFormat() == 1, "file format preserved");
        check(loaded.resolution() == 480, "resolution preserved");
        check(loaded.divisionType() == Midi::MidiFile::PPQ, "division type preserved");
        check(loaded.tracks().size() == 2, "track count preserved");
        check(loaded.events().size() == eventCount, "event count preserved");

        check(countType(loaded, Midi::MidiEvent::NoteOn) == 1, "one NoteOn event loaded");
        check(countType(loaded, Midi::MidiEvent::NoteOff) == 1, "one NoteOff event loaded");
        check(countType(loaded, Midi::MidiEvent::ControlChange) == 1, "one ControlChange event loaded");
        check(countType(loaded, Midi::MidiEvent::ProgramChange) == 1, "one ProgramChange event loaded");
        check(countType(loaded, Midi::MidiEvent::PitchWheel) == 1, "one PitchWheel event loaded");
        check(countType(loaded, Midi::MidiEvent::KeyPressure) == 1, "one KeyPressure event loaded");
        check(countType(loaded, Midi::MidiEvent::ChannelPressure) == 1, "one ChannelPressure event loaded");
        check(countType(loaded, Midi::MidiEvent::SysEx) == 1, "one SysEx event loaded");
        check(countType(loaded, Midi::MidiEvent::Meta) == 5, "five Meta events loaded");

        check(containsEvent(loaded, Midi::MidiEvent::NoteOn, 0, 0, 60, 100), "NoteOn fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::NoteOff, 480, 0, 60, -1), "NoteOff fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::ControlChange, 240, 0, 7, 100), "ControlChange fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::ProgramChange, 120, 0, 5, -1), "ProgramChange fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::PitchWheel, 360, 0, -1, 4096), "PitchWheel fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::KeyPressure, 60, 0, 60, 33), "KeyPressure fields round trip");
        check(containsEvent(loaded, Midi::MidiEvent::ChannelPressure, 90, 0, -1, 44),
              "ChannelPressure fields round trip");

        for (const auto *e : loaded.events()) {
            if (e->type() == Midi::MidiEvent::Meta && e->number() == Midi::MidiEvent::Tempo) {
                check(std::fabs(e->tempo() - 120.0f) < 0.01f, "tempo value round trip");
            }
            if (e->type() == Midi::MidiEvent::Meta && e->number() == Midi::MidiEvent::TimeSignature) {
                check(e->data() == std::vector<char>({4, 2, 24, 8}), "time signature data round trip");
            }
            if (e->type() == Midi::MidiEvent::Meta && e->number() == Midi::MidiEvent::TrackName) {
                check(e->data() == toBytes("wolf-midi"), "track name round trip");
            }
            if (e->type() == Midi::MidiEvent::SysEx) {
                check(e->data() == std::vector<char>({'\xF0', '\x7E', '\x00'}), "sysex data round trip");
            }
        }

        check(loaded.save(copyPath), "save reloaded file to second path");
        check(readFile(copyPath) == readFile(midiPath), "save/load/save produces byte-identical files");
    }

    void testStreamRoundTrip() {
        Midi::MidiFile midi;
        buildSampleMidiFile(midi);
        const auto eventCount = midi.events().size();

        std::ostringstream out;
        check(midi.save(out), "save to std::ostream");
        check(!out.str().empty(), "stream output is not empty");

        std::istringstream in(out.str());
        Midi::MidiFile loaded;
        check(loaded.load(in), "load from std::istream");
        check(loaded.events().size() == eventCount, "stream event count preserved");

        std::ostringstream out2;
        loaded.save(out2);
        check(out2.str() == out.str(), "stream output is stable across round trip");

        const std::string garbage(1024, 'x');
        std::istringstream garbageStream(garbage);
        Midi::MidiFile invalid;
        check(!invalid.load(garbageStream), "invalid data is rejected");
        check(invalid.events().empty(), "no events after failed load");
    }

    void testEventManagement() {
        Midi::MidiFile midi;
        midi.setResolution(480);
        midi.createTrack();

        auto *noteOn = midi.createNote(0, 0, 480, 0, 60, 100, 64);
        check(noteOn != nullptr && noteOn->type() == Midi::MidiEvent::NoteOn, "createNote returns the start event");
        check(midi.events().size() == 2, "createNote creates two events");
        check(countType(midi, Midi::MidiEvent::NoteOff) == 1, "createNote creates NoteOff at end tick");

        midi.removeEvent(noteOn);
        check(midi.events().size() == 1, "removeEvent removes the event");

        check(midi.beatFromTick(960) == 2.0f, "beatFromTick converts ticks to beats");
        check(midi.tickFromBeat(1.5f) == 720, "tickFromBeat converts beats to ticks");

        midi.clear();
        check(midi.events().empty() && midi.tracks().empty(), "clear removes all data");
    }

    void testOneTrackPerVoice() {
        Midi::MidiFile format0;
        format0.setFileFormat(0);
        format0.setResolution(480);
        format0.createTrack();
        format0.createNoteOnEvent(0, 0, 0, 60, 100);
        format0.createNoteOnEvent(0, 240, 1, 62, 100);
        format0.createTempoEvent(0, 0, 120.0f);

        Midi::MidiFile *split = format0.oneTrackPerVoice();
        check(split != nullptr, "oneTrackPerVoice returns a new file");
        if (split == nullptr) {
            return;
        }
        check(split->fileFormat() == 1, "oneTrackPerVoice writes format 1");
        check(split->tracks().size() == 3, "oneTrackPerVoice creates one track per voice");
        check(split->events().size() == format0.events().size(), "oneTrackPerVoice preserves all events");
        check(split->eventsForTrack(1).size() == 1, "voice 0 events moved to track 1");
        check(split->eventsForTrack(2).size() == 1, "voice 1 events moved to track 2");
        delete split;
    }
} // namespace

int main() {
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto midiPath = tempDir / "wolf-midi-test.mid";
    const auto copyPath = tempDir / "wolf-midi-test-copy.mid";

    testFileRoundTrip(midiPath, copyPath);
    testStreamRoundTrip();
    testEventManagement();
    testOneTrackPerVoice();

    std::error_code ignored;
    std::filesystem::remove(midiPath, ignored);
    std::filesystem::remove(copyPath, ignored);

    std::cout << "----------------------------------------" << std::endl;
    std::cout << g_checks << " checks, " << g_failures << " failures" << std::endl;
    if (g_failures == 0) {
        std::cout << "All tests passed." << std::endl;
        return 0;
    }
    std::cout << "TESTS FAILED." << std::endl;
    return 1;
}

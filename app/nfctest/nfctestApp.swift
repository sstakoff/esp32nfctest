//
//  nfctestApp.swift
//  nfctest
//
//  Created by Stu Stakoff on 10/12/22.
//

import SwiftUI
import CoreNFC

@main
struct nfctestApp: App {
    
    private var mr = MyReader()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(mr)
        }
    }
}

class MyReader: NSObject, NFCNDEFReaderSessionDelegate, ObservableObject {
    
    @Published var msglog: [String] = []
    
    var session: NFCReaderSession?

    
    func readerSession(_ session: NFCNDEFReaderSession, didInvalidateWithError error: Error) {
        msglog.append("Got reader error")
        self.session = nil
    }
    
    func readerSession(_ session: NFCNDEFReaderSession, didDetectNDEFs messages: [NFCNDEFMessage]) {
        msglog.append("NDEF detected!")
        msglog.append("Msgs: " + String(messages.count))
        for msg in messages {
            msglog.append("Records: " + String(msg.records.count))
            for rec in msg.records {
                if (rec.typeNameFormat != NFCTypeNameFormat.nfcWellKnown) {
                    msglog.append("Unknown record type: " + String(describing: rec.typeNameFormat))
                    continue
                }
                
                msglog.append(rec.wellKnownTypeURIPayload()?.absoluteString ?? "..not a URL")
                msglog.append(rec.wellKnownTypeTextPayload().0 ?? "..not a string")

            }
        }
    }
    
    func readerSessionDidBecomeActive(_ session: NFCNDEFReaderSession) {
        msglog.append("Reader is active")
    }
    
    func read() {
        if (!NFCReaderSession.readingAvailable) {
            msglog.append("Device does not support NFC")
            return
        }
        open()
        session?.begin()
    }
    
    private func open() {
        session = NFCNDEFReaderSession(delegate: self, queue: DispatchQueue.main, invalidateAfterFirstRead: false)
    }
    
    
    
   
    
}

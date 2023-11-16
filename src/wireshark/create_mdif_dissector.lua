do
    local protobuf_dissector = Dissector.get("protobuf")

    local name = "MDIF"
    local desc = "MyDefence Device Interface"
    local msgtype = nil
    local proto = Proto(name, desc)
    local f_length = ProtoField.uint32(name .. ".length", "Length", base.DEC)
    proto.fields = { f_length }

    get_tag = function(len, data)
        local max_rv = len
        local tag = data:get_index(0)
        local shift = 4


        tag = bit.rshift(bit.band(tag, 0x7f),3)

        if len > 5 then
            max_rv = 4
        end

        if bit.band(data:get_index(0), 0xf8) == 0 then
            return 0
        end

        if data:get_index(0) < 0x80 then
            return tag
        end

        for rv = 1, max_rv do
            if data:get_index(rv) < 0x80 then
                tag = bit.bor(tag, bit.lshift(data:get_index(rv), shift))
                return tag
            else
                tag = bit.bor(tag, bit.lshift(bit.band(data:get_index(rv), 0x7f), shift))
                shift = shift + 7
            end
        end

        return 0

    end

    proto.dissector = function(tvb, pinfo, tree)
        local subtree = tree:add(proto, tvb())
        if pinfo.port_type == 2 then -- TCP
            local offset = 0
            local remaining_len = tvb:len()

            while remaining_len > 0 do
                if remaining_len < 4 then -- head not enough
                    pinfo.desegment_offset = offset
                    pinfo.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
                    return -1
                end

                local data_len = tvb(offset, 4):le_uint()

                if remaining_len - 4 < data_len then -- data not enough
                    pinfo.desegment_offset = offset
                    pinfo.desegment_len = data_len - (remaining_len - 4)
                    return -1
                end
                subtree:add_le(f_length, tvb(offset, 4))


                local tag = get_tag(data_len, tvb(offset + 4, data_len):bytes())

                if tag < 0x20 then
                    msgtype = "mdif.core.CoreMsg"
                elseif tag < 0x30 then
                    msgtype = "mdif.fwu.FwuMsg"
                elseif tag < 0x50 then
                    msgtype = "mdif.core_dev.CoreDevMsg"
                end
                if msgtype ~= nil then
                    pinfo.private["pb_msg_type"] = "message," .. msgtype

                    pcall(Dissector.call, protobuf_dissector,
                       tvb(offset + 4, data_len):tvb(), pinfo, subtree)
                end

                offset = offset + 4 + data_len
                remaining_len = remaining_len - 4 - data_len
            end
        end
        pinfo.columns.protocol:set(name)
    end

    DissectorTable.get("tcp.port"):add(0, proto)
end
